# Space Station AI Game - Development Log

## Project Overview
A game where a neural network learns to pilot a spaceship to reach a space station while dodging bullets. The AI trains through simulation and then plays in a real-time graphical game.

---

## Session 1: Initial Setup and Training Crash Fix

Started with a basic framework but training mode was crashing immediately. Found the issue: `generateBatchData()` was creating 6-input/1-output training examples, but the neural network expected 12 inputs and 4 outputs (thrust, strafe, rotation, brake). Fixed the function to generate proper training data.

Set up compilation to output to `bin/main.exe` with O1 optimization (O2 was causing segfaults).

---

## Session 2: Training Feedback and Rotation Issues

Added win/loss reporting during training so I could see if the AI was actually winning simulations. Added `lastSimWon` and `lastSimHit` tracking to show "WIN!", "HIT", or "TIMEOUT" status.

Noticed the ship never rotates. Found multiple issues:
- Training simulation used different physics than game mode
- Rotation targets were too weak
- Dead zones were too high

Increased rotation sensitivity and lowered dead zones.

---

## Session 3: Physics Unification - The Big Problem

This was the major breakthrough. Training showed consistent wins, but the "best" model just flew in circles in the actual game. The training results weren't correlating to gameplay at all.

Discovered MAJOR physics mismatches between training and game:

| Issue | Training | Game |
|-------|----------|------|
| Ship rotation input | Always 0.0f | Used actual rotation angle |
| Angles | Absolute | Relative |
| Bullet speed | 3.0f | 5.0 |
| Fire rate | 20 frames | 30 frames |
| Velocity source | Direct variables | ship.getVelocity() |

Changed best model selection to save when AI actually WINS games instead of when validation loss improves. Also fixed model file naming - training saved to `best_model.nn` but game loaded `trained_model.nn`.

---

## Session 4: Window and UX Improvements

- Made window appear in foreground with `SetForegroundWindow()` and `SetFocus()`
- Added "press space to start" so the game doesn't start immediately
- Increased training time to 120 seconds

---

## Session 5: Complete Physics Refactor

Decided to store all SpaceShip data in the SpaceShip object for both training AND game. This way changing one changes both - no more sync issues.

Added new methods to SpaceShip class:
```cpp
void updatePosition();
void clampVelocity(float maxSpeed);
void applyDrag(float factor);
void setVelocity(Vector2D vel);
```

Updated both TrainingManager and main.cpp to use these methods identically.

---

## Session 6: The Bullet Hit Bug

Models that "won" in training still died in game. Found the critical bug:

**In training**: Getting hit by a bullet just added to loss, simulation continued. The AI learned it could tank bullets and still win!

**In game**: Getting hit ends the game immediately.

Fixed training to end simulation immediately on bullet hit - same as the real game.

---

## Session 7: Model Selection Logic

Improved how best models are saved:

1. **No wins yet**: Save model with lowest loss
2. **First win**: Save immediately as best
3. **Multiple wins**: Only save if lower loss than previous winners
4. **Non-winning after a win**: Ignored - only winners qualify now

---

## Session 8: Training Speed and Random Spawns

Wanted to train hundreds of thousands of batches, not just tens of thousands.

Changes:
- Training time: 10 minutes (was 2)
- Batch size: 32 (was 64) - smaller batches = more iterations
- O3 optimization
- Display every 5000 batches (was 1000)

Added random starting position for ship (at least 150px from station) in both training and game.

Fixed bug where ship started with non-zero velocity in game - added `ship.setVelocity(Vector2D(0, 0))` reset.

---

## Session 9: Game Loop Order Fix

Winning models STILL weren't winning in game. Did deep comparison and found the order of operations was different:

**Training order:**
1. Build sensors (see current bullet positions)
2. AI decision
3. Apply physics
4. Update ship position
5. Fire bullets
6. Update bullets & check collision

**Game order (broken):**
1. Fire bullets
2. Update bullets & check collision
3. Build sensors
4. AI decision
5. Apply physics
6. Update position

The AI was seeing bullet positions AFTER they moved in game, but BEFORE in training!

Also fixed bullet firing timing - training used counter that increments and fires when > 20, game used SpaceStation class that counts down. Made game use identical counter logic.

Changed game to load `best_model.nn` (updated live during training) instead of `trained_model.nn` (only updated at end).

---

## Session 10: Time Incentives

AI learned to just dodge bullets and run out the clock instead of approaching the station. Added time-based rewards/penalties:

| Change | Value | Effect |
|--------|-------|--------|
| Time penalty | +0.02/frame | Every frame costs points |
| Win reward | -5.0 (was -2.0) | Bigger incentive to reach station |
| Timeout penalty | +distance*0.01 | Penalized for final distance if time runs out |

Now surviving 500 frames without winning costs 10+ points, while winning quickly gives -5.0 reward. AI should aggressively pursue station now.

---

## Key Lessons Learned

1. **Training and game MUST be identical** - even small differences in physics, timing, or order of operations will cause trained models to fail.

2. **Reward shaping matters** - AI will exploit any loophole. If surviving is rewarded more than winning, it will just survive.

3. **Test with the actual game, not just loss metrics** - low validation loss means nothing if the model can't win actual games.

4. **Centralize shared logic** - putting physics in the SpaceShip class and using it everywhere prevents sync issues.

5. **Debug by comparing frame-by-frame** - when models don't transfer, the devil is in the details of execution order and timing.

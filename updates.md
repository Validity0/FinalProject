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

---

## Session 11: Tanh Activation and Output Range Fix

### The Rotation Problem
Ship was only ever rotating in one direction - the rotation output was stuck around 0.55-0.60, never going negative. This was because sigmoid activation outputs 0-1, and even with transformation `(output - 0.5) * 2`, the network wasn't learning to output values below 0.5.

### Solution: Tanh Activation
Changed the output layer to use **tanh** activation instead of sigmoid:
- Sigmoid: outputs 0 to 1
- Tanh: outputs -1 to +1 directly

Updated `Layer.h/cpp` to support a `useTanh` flag. Output layer uses tanh, hidden layers still use sigmoid.

Also had to fix backpropagation to use the correct derivative:
- Sigmoid derivative: `y * (1 - y)`
- Tanh derivative: `1 - y²`

### Training Data Updates
Training targets now use -1 to +1 range directly for strafe and rotation. Thrust and brake are stored as -1 to +1 internally, then converted back to 0-1 after prediction.

---

## Session 12: Bullet Mechanics Overhaul

### Bullet Wrapping
Bullets now wrap around screen edges instead of being deleted. This makes the game harder and more consistent - you can't just run to a corner.

### Safe Zone
Added a safe zone around the station (100px radius) where no bullets are fired. This prevents the AI from getting killed right as it reaches the goal.

### Prediction Cap Bug Fix
Bullets weren't aiming at the ship! The issue was the prediction math with slow bullet speed:
- `timeToHit = distance / BULLET_SPEED`
- With BULLET_SPEED = 2.0 and distance = 300: timeToHit = 150 frames
- With ship velocity 3: prediction offset = 3 * 150 * 0.7 = **315 pixels** (off screen!)

Fixed by capping `timeToHit` to max 60 frames. Now bullets actually lead the target properly.

---

## Session 13: Code Consolidation - GameLogic

### The Simulation/Game Mismatch
Models that won in training simulation kept failing in game mode. Even with "identical" code, there were subtle differences causing inconsistent behavior.

### Solution: Shared GameLogic Class
Created `GameLogic.h` and `GameLogic.cpp` with all shared simulation logic:
- `SimBullet` struct for lightweight bullet representation
- `SimulationResult` struct with win/hit/loss/frames
- `runSimulation()` - complete simulation loop
- `fireAtShip()` - predictive bullet firing
- `updateBullets()` - movement and wrapping
- `checkBulletCollision()` - collision detection
- `applyAIDecision()` - neural network output processing

TrainingManager now uses `GameLogic::runSimulation()` instead of its own simulation code.

### Execution Order Fix
Made sure game mode uses same order as training:
1. Fire bullets
2. Update bullets
3. Check collision
4. AI decision
5. Ship physics
6. Win check

---

## Session 14: Model Validation - Preventing Lucky Wins

### The Problem
A model could "win" by getting lucky with a favorable spawn position or bullet pattern. One lucky win would save it as the best model, wasting an entire training cycle.

### Solution: Validation Testing
When a model wins a single simulation, it must now **prove it wasn't luck**:
1. Run 10 additional simulations with different random seeds
2. Model must win at least 7/10 (70%) to be saved
3. Lucky one-off wins are rejected with "LUCKY WIN (rejected)"

This ensures only consistent performers are saved as best models.

### Settings
```cpp
const int VALIDATION_TESTS = 10;
const int VALIDATION_REQUIRED_WINS = 7;
```

---

## Key Lessons (Continued)

6. **Activation functions matter for output range** - sigmoid can't easily learn to output negative values. Use tanh when you need -1 to +1.

7. **Prediction math can overflow** - always sanity check calculations that depend on division (like time-to-hit with slow projectiles).

8. **One win means nothing** - always validate with multiple runs to filter out lucky models.

9. **Consolidate shared logic into one place** - having simulation code in multiple files guaranteed subtle differences. One source of truth is essential.

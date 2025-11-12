#include "NeuralNetwork.h"
#include <iostream>

using namespace std;

int main() {
    NeuralNetwork nn({3, 4, 4, 1});

    // One training example
    Matrix input(1, 3);
    input.data[0] = {0.9f, 0.3f, -0.7f}; // spaceship environment

    Matrix target(1, 1);
    target.data[0] = {1.0f}; // correct answer: thrust left

    float learningRate = 0.2f;

    // Training loop
    for (int i = 0; i < 10; ++i) {
        nn.train(input, target, learningRate);

        Matrix prediction = nn.predict(input);
        std::cout << "Step " << i + 1 << " prediction: ";
        prediction.print();
    }

    return 0;
}

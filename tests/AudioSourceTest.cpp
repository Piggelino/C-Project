
#include "AudioSource.hpp"

#include <iostream>
#include <string>

int main() {
    int passed = 0;
    int total = 0;

    auto test = [&](const std::string& name,
                    const std::string& actual,
                    const std::string& expected) {
        ++total;

        if (actual == expected) {
            ++passed;
            std::cout << "PASS | " << name << '\n';
        } else {
            std::cout << "FAIL | " << name
                      << " | Expected: " << expected
                      << " | Actual: " << actual << '\n';
        }
    };

    AudioSource normal("resources/pipNoise.mp3");
    test("Normal audio path",
         normal.getFilePath(), "resources/pipNoise.mp3");

    AudioSource empty("");
    test("Empty audio path",
         empty.getFilePath(), "");

    AudioSource absolute("/tmp/test.mp3");
    test("Absolute audio path",
         absolute.getFilePath(), "/tmp/test.mp3");

    AudioSource copied(normal);
    test("Copy preserves audio path",
         copied.getFilePath(), "resources/pipNoise.mp3");

    std::cout << "\nResults: " << passed << "/"
              << total << " tests passed.\n";

    return passed == total ? 0 : 1;
}

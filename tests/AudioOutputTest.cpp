
#include "AudioOutput.hpp"
#include "AudioSource.hpp"

#include <iostream>
#include <sstream>
#include <string>

int main() {
    int passed = 0;
    int total = 0;

    AudioSource source("resources/pipNoise.mp3");

    auto test = [&](const std::string& name, bool result) {
        ++total;

        if (result) {
            ++passed;
            std::cout << "PASS | " << name << '\n';
        } else {
            std::cout << "FAIL | " << name << '\n';
        }
    };

    auto playbackTriggered = [&](AudioOutput& output,
                                 double proximity,
                                 int frames) {
        std::ostringstream captured;
        auto* original = std::cout.rdbuf(captured.rdbuf());

        for (int i = 0; i < frames; ++i) {
            output.updateFrequency(source, proximity);
        }

        std::cout.rdbuf(original);

        return captured.str().find(
            "Spelar ljud med dynamisk frekvens:"
        ) != std::string::npos;
    };

    // Test 1: Minimum proximity
    {
        AudioOutput output;
        bool early = playbackTriggered(output, 0.0, 19);
        bool triggered = playbackTriggered(output, 0.0, 1);

        test("Minimum proximity waits 20 frames",
             !early && triggered);
    }

    // Test 2: Maximum proximity
    {
        AudioOutput output;

        test("Maximum proximity triggers immediately",
             playbackTriggered(output, 1.0, 1));
    }

    // Test 3: Medium proximity
    {
        AudioOutput output;
        bool early = playbackTriggered(output, 0.5, 9);
        bool triggered = playbackTriggered(output, 0.5, 1);

        test("Medium proximity waits 10 frames",
             !early && triggered);
    }

    // Test 4: Detection reset
    {
        AudioOutput output;

        bool beforeReset = playbackTriggered(output, 0.0, 19);
        output.resetDetection();

        bool early = playbackTriggered(output, 0.0, 19);
        bool triggered = playbackTriggered(output, 0.0, 1);

        test("Detection reset clears frame counter",
             !beforeReset && !early && triggered);
    }

    // Test 5: Counter resets after playback
    {
        AudioOutput output;

        bool first = playbackTriggered(output, 0.0, 20);
        bool early = playbackTriggered(output, 0.0, 19);
        bool second = playbackTriggered(output, 0.0, 1);

        test("Frame counter resets after playback",
             first && !early && second);
    }

    // Test 6: Changing proximity
    {
        AudioOutput output;

        bool early = playbackTriggered(output, 0.0, 5);
        bool triggered = playbackTriggered(output, 1.0, 1);

        test("Playback responds to changing proximity",
             !early && triggered);
    }

    std::cout << "\nResults: " << passed << "/"
              << total << " tests passed.\n";

    return passed == total ? 0 : 1;
}

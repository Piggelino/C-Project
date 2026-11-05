#pragma once

#include <string>
#include <utility>

class AudioSource {
private:
    std::string filePath;

public:
    explicit AudioSource(std::string path)
        : filePath(std::move(path)) {}

    const std::string& getFilePath() const {
        return filePath;
    }
};
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>

struct Character {
    int encoding;
    int width, height;
    int xoffset, yoffset;
    int advance;
    std::vector<std::string> bitmap;
};

bool parseBDFFile(const std::string& filename, std::vector<Character>& chars, int& fontWidth, int& fontHeight) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open BDF file: " << filename << std::endl;
        return false;
    }
    
    std::string line;
    bool inChar = false;
    bool inBitmap = false;
    Character currentChar;
    
    // Default font size from FONTBOUNDINGBOX
    fontWidth = 8;
    fontHeight = 16;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "FONTBOUNDINGBOX") {
            iss >> fontWidth >> fontHeight;
            std::cout << "Font size: " << fontWidth << "x" << fontHeight << std::endl;
        }
        else if (command == "STARTCHAR") {
            inChar = true;
            currentChar = Character();
            currentChar.width = fontWidth;
            currentChar.height = fontHeight;
        }
        else if (command == "ENCODING") {
            iss >> currentChar.encoding;
        }
        else if (command == "BBX") {
            iss >> currentChar.width >> currentChar.height >> currentChar.xoffset >> currentChar.yoffset;
        }
        else if (command == "BITMAP") {
            inBitmap = true;
            currentChar.bitmap.clear();
        }
        else if (command == "ENDCHAR") {
            inChar = false;
            inBitmap = false;
            chars.push_back(currentChar);
        }
        else if (inBitmap && !line.empty()) {
            // Remove any whitespace and add bitmap line
            std::string bitmapLine = line;
            bitmapLine.erase(std::remove_if(bitmapLine.begin(), bitmapLine.end(), ::isspace), bitmapLine.end());
            if (!bitmapLine.empty()) {
                currentChar.bitmap.push_back(bitmapLine);
            }
        }
    }
    
    return true;
}

void writePiGFXFont(const std::vector<Character>& chars, const std::string& outputFile, int fontWidth, int fontHeight) {
    std::ofstream out(outputFile, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Error: Cannot create output file: " << outputFile << std::endl;
        return;
    }
    
    // Create array for all 256 characters
    std::vector<std::vector<uint8_t>> charData(256);
    
    // Initialize all characters with empty data (all pixels off)
    for (int i = 0; i < 256; i++) {
        charData[i].resize(fontHeight * ((fontWidth + 7) / 8) * 8, 0x00);
    }
    
    // Fill in the characters we have
    for (const auto& ch : chars) {
        if (ch.encoding < 0 || ch.encoding >= 256) continue;
        
        std::vector<uint8_t>& data = charData[ch.encoding];
        
        for (int y = 0; y < fontHeight && y < (int)ch.bitmap.size(); y++) {
            const std::string& hexLine = ch.bitmap[y];
            
            // Convert hex string to bits
            for (int x = 0; x < fontWidth; x++) {
                int hexIndex = x / 4;
                int bitIndex = 3 - (x % 4);
                
                if (hexIndex < (int)hexLine.length()) {
                    char hexChar = hexLine[hexIndex];
                    int hexValue = 0;
                    if (hexChar >= '0' && hexChar <= '9') hexValue = hexChar - '0';
                    else if (hexChar >= 'A' && hexChar <= 'F') hexValue = hexChar - 'A' + 10;
                    else if (hexChar >= 'a' && hexChar <= 'f') hexValue = hexChar - 'a' + 10;
                    
                    bool pixelOn = (hexValue & (1 << bitIndex)) != 0;
                    data[y * fontWidth + x] = pixelOn ? 0xFF : 0x00;
                }
            }
        }
    }
    
    // Write all 256 characters to file
    for (int i = 0; i < 256; i++) {
        out.write(reinterpret_cast<const char*>(charData[i].data()), charData[i].size());
    }
    
    out.close();
    std::cout << "Created " << outputFile << " with " << chars.size() << " characters (" << fontWidth << "x" << fontHeight << ")" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <input.bdf> <output.bin>" << std::endl;
        return 1;
    }
    
    std::string inputFile = argv[1];
    std::string outputFile = argv[2];
    
    std::vector<Character> chars;
    int fontWidth, fontHeight;
    
    if (!parseBDFFile(inputFile, chars, fontWidth, fontHeight)) {
        return 1;
    }
    
    std::cout << "Parsed " << chars.size() << " characters from " << inputFile << std::endl;
    
    writePiGFXFont(chars, outputFile, fontWidth, fontHeight);
    
    return 0;
}

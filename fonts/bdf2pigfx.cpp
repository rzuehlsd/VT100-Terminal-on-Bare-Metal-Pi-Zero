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

int font_xoffset = 0;
int font_yoffset = 0;

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
            iss >> fontWidth >> fontHeight >> font_xoffset >> font_yoffset;
            std::cout << "Font size: " << fontWidth << "x" << fontHeight << " Offset: " << font_xoffset << "," << font_yoffset  << std::endl;
        }
        else if (command == "STARTCHAR") {
            inChar = true;
            currentChar = Character();
            currentChar.width = fontWidth;
            currentChar.height = fontHeight;
        }
        else if (command == "ENCODING") {
            iss >> currentChar.encoding;
            std::cout << "Encoding: " << currentChar.encoding << std::endl;
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
        charData[i].resize(fontWidth * fontHeight, 0x00);
    }
    
    // Fill in the characters we have
    for (const auto& ch : chars) {
        if (ch.encoding < 0 || ch.encoding >= 256) continue;

        std::vector<uint8_t>& data = charData[ch.encoding];

        // For each pixel in the glyph's BBX, place it at the correct offset in the cell
        for (int by = 0; by < ch.height && by < (int)ch.bitmap.size(); by++) {
            const std::string& hexLine = ch.bitmap[by];
            // Convert hexLine to bytes
            std::vector<uint8_t> rowBytes;
            for (size_t i = 0; i + 1 < hexLine.length(); i += 2) {
                std::string byteStr = hexLine.substr(i, 2);
                uint8_t byte = (uint8_t)strtol(byteStr.c_str(), nullptr, 16);
                rowBytes.push_back(byte);
            }
            for (int bx = 0; bx < ch.width; bx++) {
                int byteIndex = bx / 8;
                int bitIndex = 7 - (bx % 8);
                bool pixelOn = false;
                if (byteIndex < (int)rowBytes.size()) {
                    pixelOn = (rowBytes[byteIndex] & (1 << bitIndex)) != 0;
                }
                int cell_x = bx + ch.xoffset + font_xoffset;
                int cell_y = by + (fontHeight - ch.height) + font_yoffset + ch.yoffset -8;
                if (cell_x >= 0 && cell_x < fontWidth && cell_y >= 0 && cell_y < fontHeight) {
                    data[cell_y * fontWidth + cell_x] = pixelOn ? 0xFF : 0x00;
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

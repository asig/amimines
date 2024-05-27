#include <stdio.h>

#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <ctime>

#include "imgloader.h"

ImageLoader *loader;

std::string makeDefineName(const std::string& s) {
    std::string res = s;
    std::transform(res.begin(), res.end(), res.begin(), [](char c) { c= ::toupper(c); return (c<'A'||c>'Z') ? '_' : c; } );
    return res + "_H";
}

std::string toHex(std::uint16_t val) {
    char buf[7];
    sprintf(buf, "0x%04x", val);
    return buf;
}

void extractImage(int imgX, int imgY, int imgW, int imgH, const std::string& structName, std::ofstream& headerStream, std::ofstream& implStream) {
    
    int bitDepth = loader->numPlanes();
    const std::vector<std::uint8_t> image = loader->image();

    // Build the actual planes
    int planeWWords = ((imgW+15)/16); // width in words;
    std::uint16_t planes[bitDepth][planeWWords * imgH];
    for(int i = 0; i < bitDepth; i++) {
        uint8_t mask = 1 << i;
        for (int y = 0; y < imgH; y++) {
            for (int w = 0; w < planeWWords; w++) {
                planes[i][planeWWords * y + w] = 0;
                for (int x = 0; x < 16; x++) {
                    int iX = w * 16 + x;
                    if (iX >= imgW) continue;
                    uint8_t col = image[(imgY+y)*loader->w() + imgX + iX];                    
                    if (col & mask) {
                        planes[i][planeWWords * y + w] |= 1 << (15-x);
                    }
                }
            }
        }
    }

    // Header
    headerStream 
        << "#define " << structName << "_W " << imgW << "\n"
        << "#define " << structName << "_H " << imgH << "\n"
        << "extern struct Image " << structName << ";\n";

    // Impl
    // ImageData
    implStream 
        << "static USHORT CHIPMEM " << structName << "_imageData[] = {\n";
    for (int i = 0; i < bitDepth; i++) {
        implStream << "  // Plane " << i << "\n";
        for (int y = 0; y < imgH; y++) {
            implStream << "  ";
            for (int w = 0; w < planeWWords; w++) {
                implStream << toHex(planes[i][y*planeWWords + w]) << ", ";
            }
            implStream << "\n";
        }                        
    }
    implStream
        << "};\n"
        << "\n"
        << "struct Image " << structName << " = {\n"
        << "  0, 0,\n"
        << "  " << structName << "_W, " << structName << "_H,\n"
        << "  " << bitDepth << ",\n"
        << "  " << structName << "_imageData,\n"
        << "  15, 0, NULL\n"
        << "};\n"
        << "\n\n";
}

std::string datestr() {
    char buf[256];
    time_t t = time(NULL);
    struct tm * p = localtime(&t);  
    strftime(buf, sizeof(buf), "%c", p);
    return buf;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: imggen <img> <dest> " << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    std::string dest = argv[2];

    FILE *fp = fopen(filename.c_str(), "rb");
    if (!fp) {
        std::cerr << "Can't open file " << filename << std::endl;
        exit(1);
    }

    loader = getLoader(fp);
    fclose(fp);
    if (!loader) {
        std::cerr << filename << " is an unsupported image format." << std::endl;
        exit(1);
    }


    // open files
    std::ofstream headerStream;
    headerStream.open(dest + ".h");
    headerStream
        << "// Generated on " << datestr() << " from " << filename << "\n"
        << "// DO NOT MODIFY!\n\n"
        << "#ifndef " << makeDefineName(dest) << "\n"
        << "#define " << makeDefineName(dest) << "\n"
        << "#include <intuition/intuition.h>\n\n";


    std::ofstream implStream;
    implStream.open(dest + ".c");
    implStream 
        << "// Generated on " << datestr() << " from " << filename << "\n"
        << "// DO NOT MODIFY!\n\n"
        << "#include <intuition/intuition.h>\n"
        << "#include \"images.h\"\n\n"
        << "#ifdef __VBCC__\n"
        << "#define CHIPMEM __chip\n"
        << "#else\n"
        << "#define CHIPMEM  __attribute__((section (\".MEMF_CHIP\")))\n"
        << "#endif\n\n";


    int bitDepth = loader->numPlanes();

    // Digits
    int top = 0;
    extractImage( 0*13, top, 13, 23, "imgDigit0", headerStream, implStream);
    extractImage( 1*13, top, 13, 23, "imgDigit1", headerStream, implStream);
    extractImage( 2*13, top, 13, 23, "imgDigit2", headerStream, implStream);
    extractImage( 3*13, top, 13, 23, "imgDigit3", headerStream, implStream);
    extractImage( 4*13, top, 13, 23, "imgDigit4", headerStream, implStream);
    extractImage( 5*13, top, 13, 23, "imgDigit5", headerStream, implStream);
    extractImage( 6*13, top, 13, 23, "imgDigit6", headerStream, implStream);
    extractImage( 7*13, top, 13, 23, "imgDigit7", headerStream, implStream);
    extractImage( 8*13, top, 13, 23, "imgDigit8", headerStream, implStream);
    extractImage( 9*13, top, 13, 23, "imgDigit9", headerStream, implStream);
    extractImage(10*13, top, 13, 23, "imgDigitEmpty", headerStream, implStream);
    extractImage(11*13, top, 13, 23, "imgDigitDash", headerStream, implStream);
    top  +=  23;
     
    // playfield tiles
    extractImage( 0*16, top, 16, 16, "imgTile0", headerStream, implStream);
    extractImage( 1*16, top, 16, 16, "imgTile1", headerStream, implStream);
    extractImage( 2*16, top, 16, 16, "imgTile2", headerStream, implStream);
    extractImage( 3*16, top, 16, 16, "imgTile3", headerStream, implStream);
    extractImage( 4*16, top, 16, 16, "imgTile4", headerStream, implStream);
    extractImage( 5*16, top, 16, 16, "imgTile5", headerStream, implStream);
    extractImage( 6*16, top, 16, 16, "imgTile6", headerStream, implStream);
    extractImage( 7*16, top, 16, 16, "imgTile7", headerStream, implStream);
    extractImage( 8*16, top, 16, 16, "imgTile8", headerStream, implStream);
    extractImage(10*16, top, 16, 16, "imgTileMine", headerStream, implStream);
    extractImage(11*16, top, 16, 16, "imgTileMineWrong", headerStream, implStream);
    extractImage(12*16, top, 16, 16, "imgTileMineExploded", headerStream, implStream);
    extractImage(13*16, top, 16, 16, "imgTileMaybe", headerStream, implStream);
    extractImage(14*16, top, 16, 16, "imgTileFlagged", headerStream, implStream);
    extractImage(15*16, top, 16, 16, "imgTileClosed", headerStream, implStream);
    top += 16;

    // faces
    extractImage(0*24, top, 24, 24, "imgFaceNormal", headerStream, implStream);
    extractImage(1*24, top, 24, 24, "imgFaceO", headerStream, implStream);
    extractImage(2*24, top, 24, 24, "imgFaceSad", headerStream, implStream);
    extractImage(3*24, top, 24, 24, "imgFaceGlasses", headerStream, implStream);
    extractImage(4*24, top, 24, 24, "imgFacePressed", headerStream, implStream);
    top += 24;

    // logo
    extractImage(0, top, 182, 40, "imgLogo", headerStream, implStream);
    top += 40;

    // quit button
    extractImage(0*7, top, 7, 7, "btnQuit", headerStream, implStream);
    extractImage(1*7, top, 7, 7, "btnQuitPressed", headerStream, implStream);
    top += 7;

    // checkbox
    extractImage(0*7, top, 7, 7, "checkboxUnselected", headerStream, implStream);
    extractImage(1*7, top, 7, 7, "checkboxSelected", headerStream, implStream);
    top += 7;

    const std::vector<std::uint16_t> pal = loader->palette();
    int palSize = pal.size();
    headerStream << "#define NUM_COLS " << palSize << "\n";
    headerStream << "extern USHORT palette[NUM_COLS];\n\n";

    implStream << "USHORT palette[" << palSize << "] = {\n";
    for (int i = 0; i < palSize; i++) {
        implStream << "    " << toHex(pal[i]) << ",\n";
    }
    implStream << "};\n\n";


    headerStream << "#endif\n";
    headerStream.close();

    implStream.close();
    
    delete loader;

    return 0;
}

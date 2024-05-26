#include <stdio.h>
#include <png.h>

#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdint>

std::uint8_t *image;
int pngW;
int pngH;

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

void extractImage(int imgX, int imgY, int imgW, int imgH, int bitDepth, const std::string& structName, std::ofstream& headerStream, std::ofstream& implStream) {
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
                    uint8_t col = image[(imgY+y)*pngW + imgX + iX];                    
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
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);
    if(setjmp(png_jmpbuf(png))) abort();

    png_init_io(png, fp);
    png_read_info(png, info);
    pngW = png_get_image_width(png, info);
    pngH = png_get_image_height(png, info);
    png_byte colorType = png_get_color_type(png, info);
    if (colorType != PNG_COLOR_TYPE_PALETTE) {
        std::cerr << "colorType is not PNG_COLOR_TYPE_PALETTE, but " << (int)colorType << std::endl;
        exit(1);
    }
    int bitDepth = png_get_bit_depth(png, info);
    if (bitDepth != 4) {
        std::cerr << "bitDepth " << bitDepth << " not support. Supported bitDepth is 4" << std::endl;
    }

    size_t rowBytes = png_get_rowbytes(png,info);
    png_bytepp rows = new png_bytep[pngH];
    for(int y = 0; y < pngH; y++) {
        rows[y] = new png_byte[rowBytes];
    }
    png_read_image(png, rows);    
    fclose(fp);

    // Turn 4bit/pixel to 8bit/pixel
    image = new std::uint8_t[pngW * pngH];
    for (int y = 0; y < pngH; y++) {
        int x = 0;
        for (int pngX = 0; pngX < rowBytes; pngX++) {
            std::uint8_t val = rows[y][pngX];
            image[y * pngW + x++] = val >> 4;
            image[y * pngW + x++] = val & 0xf;
        }
    }

    // open files
    std::ofstream headerStream;
    headerStream.open(dest + ".h");
    headerStream
        << "#ifndef " << makeDefineName(dest) << "\n"
        << "#define " << makeDefineName(dest) << "\n"
        << "#include <intuition/intuition.h>\n\n";


    std::ofstream implStream;
    implStream.open(dest + ".c");
    implStream 
        << "#include <intuition/intuition.h>\n"
        << "#include \"images.h\"\n\n"
        << "#ifdef __VBCC__\n"
        << "#define CHIPMEM __chip\n"
        << "#else\n"
        << "#define CHIPMEM  __attribute__((section (\".MEMF_CHIP\")))\n"
        << "#endif\n\n";


    // Digits
    int top = 0;
    extractImage( 0*13,top,13,23,bitDepth, "imgDigit0", headerStream, implStream);
    extractImage( 1*13,top,13,23,bitDepth, "imgDigit1", headerStream, implStream);
    extractImage( 2*13,top,13,23,bitDepth, "imgDigit2", headerStream, implStream);
    extractImage( 3*13,top,13,23,bitDepth, "imgDigit3", headerStream, implStream);
    extractImage( 4*13,top,13,23,bitDepth, "imgDigit4", headerStream, implStream);
    extractImage( 5*13,top,13,23,bitDepth, "imgDigit5", headerStream, implStream);
    extractImage( 6*13,top,13,23,bitDepth, "imgDigit6", headerStream, implStream);
    extractImage( 7*13,top,13,23,bitDepth, "imgDigit7", headerStream, implStream);
    extractImage( 8*13,top,13,23,bitDepth, "imgDigit8", headerStream, implStream);
    extractImage( 9*13,top,13,23,bitDepth, "imgDigit9", headerStream, implStream);
    extractImage(10*13,top,13,23,bitDepth, "imgDigitEmpty", headerStream, implStream);
    extractImage(11*13,top,13,23,bitDepth, "imgDigitDash", headerStream, implStream);
    top += 23;

    // playfield items
    extractImage( 0*16,top,16,16,bitDepth, "imgTile0", headerStream, implStream);
    extractImage( 1*16,top,16,16,bitDepth, "imgTile1", headerStream, implStream);
    extractImage( 2*16,top,16,16,bitDepth, "imgTile2", headerStream, implStream);
    extractImage( 3*16,top,16,16,bitDepth, "imgTile3", headerStream, implStream);
    extractImage( 4*16,top,16,16,bitDepth, "imgTile4", headerStream, implStream);
    extractImage( 5*16,top,16,16,bitDepth, "imgTile5", headerStream, implStream);
    extractImage( 6*16,top,16,16,bitDepth, "imgTile6", headerStream, implStream);
    extractImage( 7*16,top,16,16,bitDepth, "imgTile7", headerStream, implStream);
    extractImage( 8*16,top,16,16,bitDepth, "imgTile8", headerStream, implStream);
    extractImage(10*16,top,16,16,bitDepth, "imgTileMine", headerStream, implStream);
    extractImage(11*16,top,16,16,bitDepth, "imgTileMineWrong", headerStream, implStream);
    extractImage(12*16,top,16,16,bitDepth, "imgTileMineExploded", headerStream, implStream);
    extractImage(13*16,top,16,16,bitDepth, "imgTileMaybe", headerStream, implStream);
    extractImage(14*16,top,16,16,bitDepth, "imgTileFlagged", headerStream, implStream);
    extractImage(15*16,top,16,16,bitDepth, "imgTileClosed", headerStream, implStream);
    top += 16;

    // faces
    extractImage(0*24,top,24,24,bitDepth, "imgFaceNormal", headerStream, implStream);
    extractImage(1*24,top,24,24,bitDepth, "imgFaceO", headerStream, implStream);
    extractImage(2*24,top,24,24,bitDepth, "imgFaceSad", headerStream, implStream);
    extractImage(3*24,top,24,24,bitDepth, "imgFaceGlasses", headerStream, implStream);
    extractImage(4*24,top,24,24,bitDepth, "imgFacePressed", headerStream, implStream);
    top += 24;

    // logo
    extractImage(0,top,182,40,bitDepth, "imgLogo", headerStream, implStream);
    top += 63;

    // quit button
    extractImage(0*7,top,7,7,bitDepth, "btnQuit", headerStream, implStream);
    extractImage(1*7,top,7,7,bitDepth, "btnQuitPressed", headerStream, implStream);
    top += 7;

    // checkbox
    extractImage(0*7,top,7,7,bitDepth, "checkboxUnselected", headerStream, implStream);
    extractImage(1*7,top,7,7,bitDepth, "checkboxSelected", headerStream, implStream);
    top += 7;

    // extract palette
    int numPalette;
    png_colorp palette;
    png_get_PLTE(png, info, &palette, &numPalette);

    headerStream << "#define NUM_COLS " << (1 << bitDepth) << "\n";
    headerStream << "extern USHORT palette[NUM_COLS];\n\n";

    implStream << "USHORT palette[" << (1 << bitDepth) << "] = {\n";
    for (int i = 0; i < numPalette; i++) {
        std::uint16_t r = palette[i].red/16;
        std::uint16_t g = palette[i].green/16;
        std::uint16_t b = palette[i].blue/16;
        implStream << "    " << toHex(r << 8 | g << 4 | b) << ",\n";
    }
    implStream << "};\n\n";


    headerStream << "#endif\n";
    headerStream.close();

    implStream.close();
    
    return 0;
}

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
                    uint8_t col = image[(imgY+y)*pngW + iX];
                    if (col & mask) {
                        planes[i][planeWWords * y + w] |= 1 << (16-x);
                    }
                }
            }
        }
    }

    // Header
    headerStream << "extern struct Image *" << structName << ";\n";

    // Impl
    // ImageData
    implStream 
        << "static USHORT " << structName << "_imageData[] = {\n";
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
        << "  " << imgW << ", " << imgH << ",\n"
        << "  " << bitDepth << ",\n"
        << "  " << structName << "_imageData,\n"
        << "  15, 0, NULL\n"
        << "};\n";
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
    implStream << "#include <intuition/intuition.h>\n\n";


    // Digits
    extractImage( 0*13,0,13,23,bitDepth, "img_digit0", headerStream, implStream);
    extractImage( 1*13,0,13,23,bitDepth, "img_digit1", headerStream, implStream);
    extractImage( 2*13,0,13,23,bitDepth, "img_digit2", headerStream, implStream);
    extractImage( 3*13,0,13,23,bitDepth, "img_digit3", headerStream, implStream);
    extractImage( 4*13,0,13,23,bitDepth, "img_digit4", headerStream, implStream);
    extractImage( 5*13,0,13,23,bitDepth, "img_digit5", headerStream, implStream);
    extractImage( 6*13,0,13,23,bitDepth, "img_digit6", headerStream, implStream);
    extractImage( 7*13,0,13,23,bitDepth, "img_digit7", headerStream, implStream);
    extractImage( 8*13,0,13,23,bitDepth, "img_digit8", headerStream, implStream);
    extractImage( 9*13,0,13,23,bitDepth, "img_digit9", headerStream, implStream);
    extractImage(10*13,0,13,23,bitDepth, "img_digitEmpty", headerStream, implStream);
    extractImage(11*13,0,13,23,bitDepth, "img_digitDash", headerStream, implStream);

    // playfield items
    extractImage( 0*16,23,16,16,bitDepth, "img_floor0", headerStream, implStream);
    extractImage( 1*16,23,16,16,bitDepth, "img_floor1", headerStream, implStream);
    extractImage( 2*16,23,16,16,bitDepth, "img_floor2", headerStream, implStream);
    extractImage( 3*16,23,16,16,bitDepth, "img_floor3", headerStream, implStream);
    extractImage( 4*16,23,16,16,bitDepth, "img_floor4", headerStream, implStream);
    extractImage( 5*16,23,16,16,bitDepth, "img_floor5", headerStream, implStream);
    extractImage( 6*16,23,16,16,bitDepth, "img_floor6", headerStream, implStream);
    extractImage( 7*16,23,16,16,bitDepth, "img_floor7", headerStream, implStream);
    extractImage( 8*16,23,16,16,bitDepth, "img_floor8", headerStream, implStream);
    extractImage( 9*16,23,16,16,bitDepth, "img_floorBomb", headerStream, implStream);
    extractImage(10*16,23,16,16,bitDepth, "img_floorBombWrong", headerStream, implStream);
    extractImage(11*16,23,16,16,bitDepth, "img_floorBomExploded", headerStream, implStream);
    extractImage(12*16,23,16,16,bitDepth, "img_floorMaybe", headerStream, implStream);
    extractImage(13*16,23,16,16,bitDepth, "img_floorFlagged", headerStream, implStream);
    extractImage(14*16,23,16,16,bitDepth, "img_floorClosed", headerStream, implStream);

    // faces
    extractImage(0*24,23+16,24,24,bitDepth, "img_faceNormal", headerStream, implStream);
    extractImage(1*24,23+16,24,24,bitDepth, "img_faceO", headerStream, implStream);
    extractImage(2*24,23+16,24,24,bitDepth, "img_faceSad", headerStream, implStream);
    extractImage(3*24,23+16,24,24,bitDepth, "img_faceGlasses", headerStream, implStream);
    
    headerStream << "#endif\n";
    headerStream.close();

    implStream.close();
    
    return 0;
}
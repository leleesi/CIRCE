// Read the actual decoded pixels of an export, independently of the app's font.
#include <cassert>
#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    assert(argc==4);
    const std::map<char,std::string> font{
        {'A',".#. #.# ### #.# #.#"}, {'B',"##. #.# ##. #.# ##."},
        {'D',"##. #.# #.# #.# ##."}, {'E',"### #.. ##. #.. ###"},
        {'F',"### #.. ##. #.. #.."}, {'I',"### .#. .#. .#. ###"},
        {'N',"#.# ### ### ### #.#"}, {'O',"### #.# #.# #.# ###"},
        {'P',"##. #.# ##. #.. #.."}, {'R',"##. #.# ##. #.# #.#"},
        {'S',"### #.. ### ..# ###"}, {'T',"### .#. .#. .#. .#."},
        {'0',"### #.# #.# #.# ###"}, {'1',".#. ##. .#. .#. ###"},
        {'5',"### #.. ### ..# ###"}, {' ',"... ... ... ... ..."}
    };
    // FFmpeg provides the top 32 rows (1280 pixels wide) as decoded grayscale.
    std::vector<unsigned char> pixels(1280*32);
    std::ifstream frame(argv[1],std::ios::binary);
    assert(frame.read(reinterpret_cast<char*>(pixels.data()),pixels.size()));
    for (unsigned side=0;side<2;++side) {
        const std::string expected=argv[2+side];
        for (unsigned i=0;i<expected.size();++i) {
            auto& glyph=font.at(expected[i]);
            for (unsigned y=0;y<5;++y)
                for (unsigned x=0;x<3;++x) {
                    unsigned left=side*640+17+i*8+x*2, top=15+y*2;
                    int luma=pixels[top*1280+left]+pixels[top*1280+left+1]
                           +pixels[(top+1)*1280+left]+pixels[(top+1)*1280+left+1];
                    if ((luma > 4*145) != (glyph[y*4+x]=='#')) {
                        std::fprintf(stderr,"Unexpected pixels in '%s', character %u, row %u, column %u\n",
                                     expected.c_str(),i,y,x);
                        return 1;
                    }
                }
        }
    }
}

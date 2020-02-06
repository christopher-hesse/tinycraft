package main

import (
	"bufio"
	"bytes"
	"encoding/hex"
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"os"
	"path"
	"path/filepath"
	"strconv"
	"strings"
)

// https://en.wikipedia.org/wiki/Glyph_Bitmap_Distribution_Format

var srcFile io.Writer

const chars = 128

type FontWeight int

const (
	weightMedium FontWeight = iota
	weightBold
)

type Font struct {
	Width  int
	Height int
	Weight FontWeight
	Glyphs map[rune]Glyph
}

type Glyph struct {
	Encoding rune
	Bitmap   []bool
}

func visit(filePath string, f os.FileInfo, err error) error {
	if f.IsDir() {
		return nil
	}

	if path.Ext(f.Name()) != ".bdf" {
		return nil
	}

	contents, err := ioutil.ReadFile(filePath)
	if err != nil {
		return err
	}

	font := Font{Glyphs: map[rune]Glyph{}}
	glyph := Glyph{}

	scanner := bufio.NewScanner(bytes.NewReader(contents))

	for scanner.Scan() {
		line := scanner.Text()
		parts := strings.Split(line, " ")
		switch parts[0] {
		case "STARTCHAR":
			glyph = Glyph{}
		case "WEIGHT_NAME":
			switch parts[1] {
			case `"Bold"`:
				font.Weight = weightBold
			case `"Medium"`:
				font.Weight = weightMedium
			default:
				return errors.New("invalid weight")
			}
		case "ENCODING":
			encoding, err := strconv.Atoi(parts[1])
			if err != nil {
				return err
			}
			glyph.Encoding = rune(encoding)
		case "FONTBOUNDINGBOX":
			font.Width, err = strconv.Atoi(parts[1])
			if err != nil {
				return err
			}
			font.Height, err = strconv.Atoi(parts[2])
			if err != nil {
				return err
			}
		case "BITMAP":
			for i := 0; i < font.Height; i++ {
				if !scanner.Scan() {
					break
				}
				b, err := hex.DecodeString(scanner.Text())
				if err != nil {
					return err
				}
				for j := 0; j < font.Width; j++ {
					offset := 7 - uint(j%8)
					bit := int(b[j/8]) & (1 << offset) >> offset
					glyph.Bitmap = append(glyph.Bitmap, bit == 1)
				}
			}
		case "ENDCHAR":
			font.Glyphs[glyph.Encoding] = glyph
		}
	}

	if err := scanner.Err(); err != nil {
		return err
	}

	bytes := make([]bool, font.Width*font.Height*chars)

	for r := 0; r < chars; r++ {
		g, ok := font.Glyphs[rune(r)]
		if !ok {
			continue
		}

		for y := 0; y < font.Height; y++ {
			for x := 0; x < font.Width; x++ {
				if g.Bitmap[y*font.Width+x] {
					offset := (font.Height-y-1)*font.Width*chars + r*font.Width + x
					bytes[offset] = true
				}
			}
		}
	}

	texture := make([]byte, font.Width*font.Height*chars*4)

	for i, b := range bytes {
		if b {
			offset := i * 4
			texture[offset] = 255
			texture[offset+1] = 255
			texture[offset+2] = 255
			texture[offset+3] = 255
		}
	}

	rletexture := []uint16{}

	prev_byte := byte(0)
	count := 0
	for _, b := range texture {
		if count > 65535 {
			panic("oh no")
		}
		if b != prev_byte {
			rletexture = append(rletexture, uint16(count))
			count = 0
		}
		prev_byte = b
		count++
	}
	if count > 0 {
		rletexture = append(rletexture, uint16(count))
	}
	rletexture = append(rletexture, 0)

	weight := ""

	switch font.Weight {
	case weightMedium:
		weight = "Medium"
	case weightBold:
		weight = "Bold"
	}

	name := fmt.Sprintf("Terminus%d%s", font.Height, weight)

	fmt.Fprintf(srcFile, "uint16_t %sCompressedData[] = {\n", name)
	for i, b := range rletexture {
		if i == len(rletexture)-1 {
			fmt.Fprintf(srcFile, "%d", b)
		} else {
			fmt.Fprintf(srcFile, "%d,", b)
			if i%10 == 9 {
				fmt.Fprintf(srcFile, "\n")
			} else {
				fmt.Fprintf(srcFile, " ")
			}
		}
	}
	fmt.Fprintf(srcFile, "};\n\n")

	fmt.Fprintf(srcFile, "Font %s = {\n", name)
	fmt.Fprintf(srcFile, "  %d,\n", font.Width)
	fmt.Fprintf(srcFile, "  %d,\n", font.Height)
	fmt.Fprintf(srcFile, "  %d,\n", chars)
	fmt.Fprintf(srcFile, "  load_compressed_data(%d, %sCompressedData),\n", len(texture), name)
	fmt.Fprintf(srcFile, "};\n\n")

	return nil
}

func run() error {
	var err error

	srcFile, err = os.Create("font.c")
	if err != nil {
		return err
	}

	fmt.Fprintf(srcFile,
		`#include <stdlib.h>

uint8_t *load_compressed_data(uint32_t length, uint16_t *CompressedData) {
    auto buf = new uint8_t[length];
    int buf_offset = 0;
    int data_offset = 0;
    uint8_t byte;
    while (true) {
        int run_length = CompressedData[data_offset];
        if (data_offset %% 2 == 0) {
            byte = 0;
        } else {
            byte = 0xFF;
        }
        if (run_length == 0) {
            break;
        }
        for (int i = 0; i < run_length; i++) {
            buf[buf_offset] = byte;
            buf_offset++;
        }
        data_offset++;
    }
    return buf;
}

typedef struct {
    int width;
    int height;
    int chars;
    uint8_t *data;
} Font;

`)

	if err := filepath.Walk("font", visit); err != nil {
		return err
	}
	return nil
}

func main() {
	if err := run(); err != nil {
		log.Fatal(err)
	}
}

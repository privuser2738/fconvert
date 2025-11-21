# fconvert - Development Roadmap

## Project Vision
Create an enterprise-grade, cross-platform file conversion tool with support for 100+ file formats across multiple categories, all implemented in pure C/C++ without external dependencies where feasible.

---

## ✅ Phase 1: Foundation & Core Formats (COMPLETED)

### Infrastructure (COMPLETED)
- ✅ Cross-platform build system (CMake)
- ✅ CLI argument parser (30+ options)
- ✅ Configuration file system (.ini)
- ✅ Plugin-based converter architecture
- ✅ File type detection (magic numbers + extensions)
- ✅ Batch processing (files, folders, recursive)
- ✅ Professional logging system
- ✅ Progress reporting with statistics

### Compression Algorithms (COMPLETED - ~900 lines)
- ✅ CRC32 checksum
- ✅ LZ77 sliding window compression (32KB window, hash-based)
- ✅ Huffman coding (canonical trees, fixed/dynamic codes)
- ✅ DEFLATE compression (RFC 1951) - full implementation

### Image Formats (COMPLETED - ~1,400 lines)
- ✅ **BMP** (24/32-bit RGB/RGBA)
- ✅ **PNG** (full codec with DEFLATE, all 5 filter types)
- ✅ **Image transformations** (resize, rotate, flip)
  - 3 interpolation methods (nearest, bilinear, bicubic)
  - Rotation (90°, 180°, 270°)
  - Horizontal/vertical flip

### Archive Formats (COMPLETED - ~1,100 lines)
- ✅ **GZIP** (.gz) - RFC 1952
- ✅ **TAR** (.tar) - POSIX ustar
- ✅ **ZIP** (.zip) - PKZIP with DEFLATE
- ✅ **TAR.GZ** (.tgz) - Combined format
- ✅ Full archive↔archive conversions (TAR↔ZIP↔TGZ↔GZ)

**Total Phase 1**: ~11,500+ lines of production C/C++ code

---

## 🚧 Phase 2: Extended Formats (IN PROGRESS)

### Priority: Simple Image Formats
- 🚧 **TGA** (.tga) - Targa format (uncompressed, RLE)
- 🚧 **PPM/PGM/PBM** - Netpbm formats (ASCII, binary)
- ⏳ **GIF** (.gif) - LZW compression required
- ⏳ **ICO** (.ico) - Windows icon format
- ⏳ **PCX** - PC Paintbrush format

**Estimated**: ~800 lines

### Priority: Additional Archive Formats
- 🚧 **TAR.BZ2** (.tar.bz2, .tbz2) - Requires BZ2 implementation
- 🚧 **TAR.XZ** (.tar.xz, .txz) - Requires LZMA implementation
- ⏳ **7-Zip** (.7z) - Complex, requires LZMA/LZMA2
- ⏳ **RAR** (extract only) - Proprietary, complex
- ⏳ **ISO 9660** - CD/DVD image format

**Note**: 7z and advanced compression require significant work (LZMA/LZMA2 ~2000+ lines)

### Priority: Document Conversions
- 🚧 **TXT** (.txt) - Plain text
- 🚧 **Markdown** (.md) - Markdown format
- 🚧 **CSV** (.csv) - Comma-separated values
- ⏳ **JSON** (.json) - JSON format
- ⏳ **XML** (.xml) - XML format
- ⏳ **YAML** (.yaml) - YAML format
- ⏳ **HTML** (basic) - HTML to/from markdown

**Estimated**: ~600 lines for text-based formats

---

## 📅 Phase 3: Advanced Image Formats (FUTURE)

### Lossy Compression
- ⏳ **JPEG** (.jpg/.jpeg) - DCT + Huffman + quantization
  - Difficulty: ⭐⭐⭐⭐⭐ (Very High)
  - Estimated: 2,000-3,000 lines
  - Timeline: 1-2 weeks
  - Components: DCT, YCBCR conversion, quantization tables, Huffman

- ⏳ **WebP** (.webp) - VP8/VP9 codec
  - Difficulty: ⭐⭐⭐⭐⭐⭐ (Expert)
  - Estimated: 3,000-5,000 lines
  - Timeline: 2-3 weeks
  - Note: Extremely complex, may require libwebp

### High Dynamic Range
- ⏳ **TIFF** (.tiff/.tif) - Multiple compression methods
  - Difficulty: ⭐⭐⭐⭐ (High)
  - Estimated: 1,500 lines
  - Supports: Uncompressed, LZW, DEFLATE, JPEG

- ⏳ **EXR** - OpenEXR format
  - Difficulty: ⭐⭐⭐⭐⭐ (Very High)
  - Note: May require OpenEXR library

---

## 🎬 Phase 4: Video/Audio Conversion (COMPLEX)

### Architecture Approach
Video conversion is **extremely complex** and typically requires:
- **Container parsing** (MP4, AVI, MKV, WebM)
- **Codec implementations** (H.264, H.265, VP8, VP9, AV1)
- **Audio codecs** (AAC, MP3, Opus, Vorbis)
- **Frame processing** (decoding, encoding, filtering)
- **Synchronization** (A/V sync, timestamps)

### Realistic Implementation Strategy

#### Option 1: Native Implementation (Pure C/C++)
**Extremely difficult** - would require:
- H.264 decoder (~10,000+ lines)
- H.264 encoder (~15,000+ lines)
- MP4 container parser (~2,000 lines)
- Audio codecs (~5,000+ lines each)
- **Total**: 40,000-60,000 lines minimum
- **Timeline**: 6-12 months for basic support

#### Option 2: FFmpeg Wrapper (Recommended)
- Use FFmpeg libraries (avcodec, avformat, avutil)
- Implement C++ wrapper layer (~1,000 lines)
- Support all major formats via FFmpeg
- **Timeline**: 1-2 weeks
- **Trade-off**: External dependency

#### Option 3: Hybrid Approach
- Implement simple formats natively:
  - **WAV** ↔ **FLAC** (lossless audio)
  - **AVI** (uncompressed) parser
  - **Raw video** formats (YUV, RGB)
- Use FFmpeg for complex codecs

### Proposed Video Roadmap

#### Phase 4A: Audio Formats (Native)
- ⏳ **WAV** (.wav) - RIFF WAVE (uncompressed PCM)
  - Estimated: ~300 lines
  - Supports: PCM, basic metadata

- ⏳ **FLAC** (.flac) - Lossless audio compression
  - Estimated: ~1,500 lines
  - LPC prediction + Rice coding

- ⏳ **OGG Vorbis** (.ogg) - Requires libvorbis or ~5,000 lines

#### Phase 4B: Simple Video (Container Only)
- ⏳ **AVI** (.avi) - Parse RIFF structure, extract frames
  - Estimated: ~800 lines
  - Note: Decoding codecs requires much more work

#### Phase 4C: FFmpeg Integration (If Needed)
- ⏳ FFmpeg wrapper for complex conversions
- ⏳ Support: MP4, MKV, WebM, MOV, FLV
- ⏳ Codecs: H.264, H.265, VP8, VP9, AV1

---

## 🎨 Phase 5: 3D Model Conversion (COMPLEX)

### Architecture Overview
3D model conversion requires:
- **Mesh data**: Vertices, faces, normals, UVs
- **Material systems**: Textures, shaders, properties
- **Scene graphs**: Hierarchies, transformations
- **Animation data**: Keyframes, bone weights

### Difficulty Assessment

#### Simple Formats (Feasible)
- ⏳ **OBJ** (.obj) - Wavefront OBJ
  - Difficulty: ⭐⭐ (Medium)
  - Estimated: ~500 lines
  - Text-based, widely supported
  - Supports: Vertices, normals, UVs, faces

- ⏳ **STL** (.stl) - Stereolithography
  - Difficulty: ⭐ (Easy)
  - Estimated: ~200 lines
  - 3D printing format
  - Supports: Triangle meshes only

- ⏳ **PLY** (.ply) - Polygon File Format
  - Difficulty: ⭐⭐ (Medium)
  - Estimated: ~400 lines
  - ASCII or binary

#### Complex Formats (Very Difficult)
- ⏳ **FBX** (.fbx) - Autodesk FBX
  - Difficulty: ⭐⭐⭐⭐⭐⭐ (Expert)
  - Estimated: 5,000-10,000 lines
  - Proprietary binary format
  - Note: May require FBX SDK

- ⏳ **Collada** (.dae) - XML-based
  - Difficulty: ⭐⭐⭐⭐ (High)
  - Estimated: ~2,000 lines
  - Complex XML schema

- ⏳ **glTF** (.gltf/.glb) - GL Transmission Format
  - Difficulty: ⭐⭐⭐⭐ (High)
  - Estimated: ~1,500 lines
  - JSON + binary buffers
  - Modern, growing support

- ⏳ **BLEND** (.blend) - Blender native
  - Difficulty: ⭐⭐⭐⭐⭐⭐ (Expert)
  - Note: Extremely complex, recommend Blender API

### Proposed 3D Roadmap

#### Phase 5A: Simple Mesh Formats
1. **OBJ** ↔ **STL** conversion (~700 lines)
2. **PLY** support (~400 lines)
3. Basic mesh operations (scale, rotate, translate)

#### Phase 5B: Intermediate Formats
1. **glTF** 2.0 support (~1,500 lines)
2. **Collada** (.dae) basic support (~2,000 lines)

#### Phase 5C: Advanced (Library-Based)
1. **FBX** via FBX SDK
2. **BLEND** via Blender Python API

---

## 📄 Phase 6: Document Conversion (MIXED)

### Text-Based Formats (Native - Easy)

#### Phase 6A: Plain Text & Markup
- 🚧 **TXT** ↔ **Markdown** conversion
- 🚧 **CSV** ↔ **JSON** conversion
- ⏳ **XML** ↔ **JSON** conversion
- ⏳ **YAML** ↔ **JSON** conversion
- ⏳ **INI** ↔ **JSON** conversion
- ⏳ **HTML** (simple) ↔ **Markdown**

**Estimated**: ~1,000 lines total

#### Phase 6B: Structured Data
- ⏳ **SQLite** database export to CSV/JSON
- ⏳ **Protocol Buffers** (.proto)
- ⏳ **MessagePack** (.msgpack)

### Binary Office Formats (Very Difficult)

#### DOCX/XLSX/PPTX (OpenXML)
- Difficulty: ⭐⭐⭐⭐⭐ (Very High)
- These are ZIP archives containing XML + relationships
- Estimated: 3,000-5,000 lines per format
- Realistic approach:
  - Extract text content only
  - Use libraries for full conversion (Apache POI, docx4j)

#### PDF
- Difficulty: ⭐⭐⭐⭐⭐⭐ (Expert)
- Extremely complex format
- Realistic options:
  - **PDF to text**: ~2,000 lines (basic)
  - **PDF generation**: Use PDFlib or libHaru
  - **Full PDF**: 10,000+ lines

#### RTF (Rich Text Format)
- Difficulty: ⭐⭐⭐ (Moderate)
- Estimated: ~1,000 lines
- Feasible for basic conversion

### Proposed Document Roadmap

#### Phase 6A: Text Formats (Native)
1. TXT, Markdown, CSV, JSON (simple conversions)
2. XML, YAML parsing and generation
3. HTML to Markdown converter

#### Phase 6B: RTF Support
1. RTF reader (~600 lines)
2. RTF writer (~400 lines)
3. RTF ↔ TXT/Markdown

#### Phase 6C: Office Formats (Extract Only)
1. DOCX text extraction (ZIP + XML parsing)
2. XLSX data extraction
3. PDF text extraction (basic)

---

## 🗺️ Priority Matrix

### High Priority (Next 2-4 weeks)
1. ✅ Complete Phase 1 (DONE)
2. 🚧 **TGA + Netpbm formats** (simple, adds 5+ formats)
3. 🚧 **Text document conversions** (TXT, MD, CSV, JSON)
4. 🚧 **TAR.BZ2 and TAR.XZ** (requires compression codecs)
5. 📝 **Comprehensive testing suite**

### Medium Priority (1-2 months)
1. **JPEG codec** (very complex but valuable)
2. **TIFF format** (multi-format container)
3. **OBJ ↔ STL 3D conversion** (simple, useful)
4. **FLAC audio** (lossless)
5. **RTF documents**

### Low Priority (Future)
1. **WebP** (very complex)
2. **Video conversion** (requires FFmpeg or massive work)
3. **FBX/BLEND** (requires external libraries)
4. **PDF generation** (use library)
5. **Office formats** (complex, consider libraries)

---

## 📊 Implementation Estimates

### Lines of Code by Category

| Category | Completed | Planned | Total Estimated |
|----------|-----------|---------|-----------------|
| **Core Infrastructure** | 6,000 | 500 | 6,500 |
| **Compression Algorithms** | 900 | 3,000 | 3,900 |
| **Image Formats** | 1,400 | 4,000 | 5,400 |
| **Archive Formats** | 1,100 | 2,000 | 3,100 |
| **Document Formats** | 0 | 3,000 | 3,000 |
| **Audio Formats** | 0 | 2,000 | 2,000 |
| **Video Formats** | 0 | 1,000 * | 1,000 * |
| **3D Model Formats** | 0 | 3,000 | 3,000 |
| **Testing & Utilities** | 200 | 1,500 | 1,700 |
| **TOTAL** | **~11,500** | **~20,000** | **~31,500** |

_* Video formats assume FFmpeg wrapper; native implementation would be 40,000+ lines_

---

## 🎯 Realistic Goals

### 3 Month Target
- Complete simple image formats (TGA, Netpbm, GIF, ICO)
- Text document conversions (TXT, MD, CSV, JSON, XML, YAML)
- Basic audio (WAV, FLAC)
- Simple 3D models (OBJ, STL, PLY)
- **Total**: ~20,000 lines

### 6 Month Target
- JPEG codec implementation
- TIFF format support
- Advanced archive formats
- RTF documents
- glTF 3D format
- **Total**: ~28,000 lines

### 1 Year Target
- Video conversion (via FFmpeg wrapper)
- Office format text extraction
- Advanced image formats (WebP consideration)
- Comprehensive format coverage
- **Total**: 35,000-40,000 lines

---

## 🚀 Next Steps (Immediate)

1. ✅ Complete archive formats (DONE)
2. 🚧 **Implement TGA format** (~300 lines, 2 hours)
3. 🚧 **Implement Netpbm formats** (PPM/PGM/PBM) (~400 lines, 2 hours)
4. 🚧 **Text document converter** (TXT, MD, CSV) (~600 lines, 3 hours)
5. 📝 **Update FEATURES_IMPLEMENTED.md**
6. 📝 **Create comprehensive test suite**

---

## 💡 Key Design Principles

1. **Pure C/C++ first**: Implement natively where feasible
2. **No dependencies preferred**: Only use libraries for extremely complex formats
3. **Enterprise quality**: Professional error handling, logging, testing
4. **Modular architecture**: Plugin-based, easy to extend
5. **Cross-platform**: Windows, Linux, macOS support
6. **Performance**: Optimize hot paths, efficient algorithms
7. **Comprehensive**: Better to do fewer formats well than many poorly

---

## 📚 Technical References

### Compression
- RFC 1950: ZLIB format
- RFC 1951: DEFLATE compression
- RFC 1952: GZIP format
- RFC 7932: Brotli compression
- Wikipedia: LZW, LZMA, BZ2

### Image Formats
- PNG Specification: ISO/IEC 15948
- JPEG Standard: ITU-T T.81
- WebP: Google WebP documentation
- TGA: Truevision TGA spec

### 3D Models
- OBJ: Wavefront OBJ specification
- STL: 3D Systems STL format
- glTF 2.0: Khronos Group specification
- Collada 1.5: ISO/PAS 17506

### Documents
- RTF 1.9: Microsoft RTF specification
- OpenXML: ISO/IEC 29500
- PDF 1.7: ISO 32000-1:2008

---

*Last Updated: 2025-11-21*
*Total Project Size: ~11,500 lines and growing*

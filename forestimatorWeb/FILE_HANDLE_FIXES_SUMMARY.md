# File Handle Fixes Summary

## Problem
Boost::filesystem error: "Too many files open" - indicating unclosed file handles in the codebase.

## Root Causes Found and Fixed

### 1. **rasterclipresource.cpp** ⚠️ CRITICAL
**Issues Fixed:**
- **Line 36**: `pInputRaster` was opened with `GDALOpen()` but NOT checked for NULL before use
- **Line 46**: `const char *out` was pointing to a temporary string that goes out of scope
- **Line 119**: When `transform[1] < 10`, `pInputRaster` was NEVER closed (memory leak in error path)
- **CSL Options Leak**: `papszOptions` created via `CSLSetNameValue()` but never freed with `CSLDestroy()`

**Fixes Applied:**
- Changed `const char *inputPath = pathTif.c_str()` to `std::string inputPath = pathTif` to keep string alive
- Changed `const char *out` to `std::string output` to maintain lifetime
- Added NULL check after `GDALOpen()`: returns early if file can't be opened
- Added `GDALClose(pInputRaster)` in the error branch (line 119+)
- Updated all references from `out` to `output.c_str()` or `output`
- Added cleanup at end: `CSLDestroy(papszOptions)` if not NULL

### 2. **wopenlayers.cpp**
**Issues Fixed:**
- **Line 14**: `std::ifstream t` was never explicitly closed

**Fixes Applied:**
- Added `t.close()` after reading stream to file

### 3. **parcellaire.cpp** ⚠️ CRITICAL LOGIC BUG
**Issues Fixed:**
- **Line 201**: `GDALClose(DS)` called unconditionally but `DS` could be NULL if `GDALOpenEx` failed
- **Line 567**: **LOGIC BUG** - `GDAL_OF_VECTOR || GDAL_OF_READONLY` (logical OR) should be `|` (bitwise OR)
  - This would cause incorrect flag parsing in GDALOpenEx

**Fixes Applied:**
- Added NULL check: `if (DS != NULL) { GDALClose(DS); }`
- Fixed operator from `||` to `|` for proper flag combination

### 4. **layerstatchart.cpp**, **manipRessource.cpp**, **staticmapresource.cpp**
**Status**: Already properly closing all file handles with explicit `.close()` calls

## Additional Observations

### Files Checked - Already Handling Resources Correctly:
- `api/staticmapresource.cpp` - Line 32 has explicit `.close()`
- `manipRessource.cpp` - Lines 120 and 127 have explicit `.close()` 
- `layerstatchart.cpp` - Lines 310, 365, 389 have explicit `.close()`
- `ACR/formviellecouperase.cpp` - Proper GDALClose calls
- `parcellaire.cpp` (other methods) - Most methods properly close GDAL datasets

### Files Not Changed But Worth Monitoring:
- `desserteForest/desserteForest.cpp` - Uses GDAL, appeared to close properly
- `OGF/formOGF.cpp` - Uses GDAL, appeared to close properly
- `grouplayers.cpp` - Most GDAL operations close properly, but `getDSonEnv()` at line 1064 creates virtual dataset that may need cleanup

## Recommendations

1. **Immediate**: Test the application thoroughly after these fixes
2. **Short-term**: Implement RAII wrappers for GDAL dataset handles to prevent future leaks
3. **Medium-term**: Use smart pointers (std::unique_ptr with custom deleters) for GDAL resources
4. **Long-term**: Consider using GDAL's C++ bindings if available, rather than C API calls

## Implementation Details

### rasterclipresource.cpp Changes:
```cpp
// BEFORE:
const char *inputPath = pathTif.c_str();  // String goes out of scope!
const char *out = output.c_str();          // Same problem
GDALClose(DS);  // Called even if DS is NULL

// AFTER:
std::string inputPath = pathTif;          // Keep string alive
std::string output = mDico->File("TMPDIR") + "/..." + ".tif";
if (pInputRaster == NULL) {
    // Early return with error
}
// ... use output.c_str() or output as needed ...
if (papszOptions != NULL) {
    CSLDestroy(papszOptions);  // Clean up options
}
```

### parcellaire.cpp Changes:
```cpp
// BEFORE:
GDALDataset *mDS = (GDALDataset *)GDALOpenEx(..., GDAL_OF_VECTOR || GDAL_OF_READONLY, ...);
GDALClose(DS);  // Unconditional, might be NULL

// AFTER:
GDALDataset *mDS = (GDALDataset *)GDALOpenEx(..., GDAL_OF_VECTOR | GDAL_OF_READONLY, ...);
if (DS != NULL) {
    GDALClose(DS);
}
```

## Testing
Monitor system for:
- Reduced file descriptor usage
- Smoother application performance during long operations
- No "Too many files open" errors
- Check `/proc/[pid]/fd` on Linux or `lsof -p [pid]` on macOS to verify file handles are being closed
- Monitor `/proc/[pid]/limits` to track open file descriptor limits

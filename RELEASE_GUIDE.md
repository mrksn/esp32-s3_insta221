# ESP32-S3 Insta Retrofit Release Guide

## Overview
This guide explains how to create and distribute releases of the ESP32-S3 Insta Retrofit firmware.

## Quick Release Process

### 1. Prepare Your Environment
```bash
# Ensure ESP-IDF is installed and in PATH
# Build the project
idf.py build
```

### 2. Create Release Package
```bash
# Run the release script with desired version
./create_release.sh v1.0.0

# Verify the firmware
cd release-v1.0.0
./verify.sh
```

### 3. Create Git Tag
```bash
# Create annotated tag
git tag -a v1.0.0 -m "Release v1.0.0: [Brief description]"

# Push tag to repository
git push origin v1.0.0
```

### 4. Create GitHub Release
1. Go to your repository on GitHub
2. Click "Releases" → "Create a new release"
3. Select tag `v1.0.0`
4. Title: "Release v1.0.0"
5. Copy content from `RELEASE_NOTES.md`
6. Upload `release-v1.0.0.tar.gz`
7. Publish release

## Release Contents

Each release package includes:

```
release-v1.0.0/
├── firmware/              # Binary firmware files
│   ├── insta-retrofit.bin      # Main application
│   ├── bootloader.bin           # ESP32 bootloader
│   ├── partition-table.bin      # Flash partitioning
│   └── flasher_args.json        # Flash configuration
├── flash.sh               # Linux/macOS flashing script
├── flash.bat              # Windows flashing script
├── platformio-flash.py    # PlatformIO compatible script
├── verify.sh              # Firmware verification script
├── README.md              # Full documentation
├── RELEASE_NOTES.md       # Release-specific notes
├── QUICK_REFERENCE.md     # Quick start guide
├── version.txt            # Build information
└── checksums.sha256       # File integrity verification
```

## Flashing Instructions for Users

### Linux/macOS
```bash
# Extract release
tar -xzf release-v1.0.0.tar.gz
cd release-v1.0.0

# Verify firmware
./verify.sh

# Flash to device
./flash.sh
```

### Windows
```cmd
REM Extract release and navigate to directory
REM Run verification
verify.sh

REM Flash firmware
flash.bat
```

### PlatformIO
```python
# Place platformio-flash.py in your project
# Run with:
pio run -t upload --upload-port /dev/ttyUSB0
```

## Firmware Specifications

- **Target**: ESP32-S3-WROOM-1-N8
- **Flash Size**: 2MB
- **Flash Mode**: DIO (Dual I/O)
- **Flash Frequency**: 80MHz
- **ESP-IDF Version**: 5.5.1

## Quality Assurance

### Pre-Release Checklist
- [ ] Project builds without errors
- [ ] All unit tests pass (`idf.py test`)
- [ ] Firmware verification passes (`./verify.sh`)
- [ ] Hardware testing completed
- [ ] Documentation updated
- [ ] Release notes written

### Post-Release Verification
- [ ] GitHub release created
- [ ] Tag pushed to repository
- [ ] Release archive downloadable
- [ ] Flashing scripts tested on target platforms
- [ ] User feedback collected

## Version Numbering

Follow [Semantic Versioning](https://semver.org/):
- **MAJOR**: Breaking changes
- **MINOR**: New features (backward compatible)
- **PATCH**: Bug fixes (backward compatible)

Examples:
- `v1.0.0`: Initial stable release
- `v1.1.0`: New features added
- `v1.1.1`: Bug fixes
- `v2.0.0`: Breaking changes

## Distribution Channels

1. **GitHub Releases**: Primary distribution
2. **Project Website**: If applicable
3. **Package Managers**: Future consideration
4. **Community Forums**: Announce releases

## Support and Troubleshooting

### Common Issues
- **Permission denied**: Check udev rules (see UDEV_RULES_ANALYSIS.md)
- **Device not found**: Verify USB connection and drivers
- **Flash failed**: Check power supply and USB cable quality
- **Firmware corrupted**: Re-download and verify checksums

### Getting Help
1. Check `QUICK_REFERENCE.md`
2. Review `RELEASE_NOTES.md`
3. Check GitHub Issues
4. Contact maintainers

## Maintenance

### Updating Releases
1. Make code changes
2. Update version numbers in relevant files
3. Test thoroughly
4. Create new release following this guide

### Deprecating Releases
- Mark old releases as deprecated in GitHub
- Update documentation to reference newer versions
- Maintain download links for critical fixes

---

**Remember**: Always test releases on actual hardware before publishing!
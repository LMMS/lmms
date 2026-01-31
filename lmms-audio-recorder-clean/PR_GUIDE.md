# LMMS Audio Recording - Pull Request Guide

## Quick Steps to Submit Your PR

### Step 1: Fork LMMS
```bash
# Go to https://github.com/LMMS/lmms
# Click "Fork" button (top right)
# This creates: https://github.com/mmmfchal/lmms
```

### Step 2: Clone Your Fork
```bash
git clone https://github.com/mmmfchal/lmms.git
cd lmms
```

### Step 3: Create Feature Branch
```bash
git checkout -b feature/audio-recording-alsa-pipewire
```

### Step 4: Add Your Files

You have two options:

#### Option A: Standalone Module (Recommended)
Copy your files to a new location:
```bash
# Create directory for audio recording module
mkdir -p src/core/recording
mkdir -p include/recording

# Copy headers
cp /path/to/lmms-audio-recorder-clean/include/*.h include/recording/

# Copy source files
cp /path/to/lmms-audio-recorder-clean/src/*.cpp src/core/recording/
```

#### Option B: Integrate with Existing Files
Modify existing LMMS files to add recording support:
- Edit `src/core/AudioEngine.cpp` to add input handling
- Edit `src/core/audio/AudioDevice.cpp` base class
- Add UI elements to `src/gui/editors/SongEditor.cpp`

### Step 5: Update CMakeLists.txt

Add to `CMakeLists.txt`:
```cmake
# Find audio libraries
find_package(ALSA)
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(PIPEWIRE libpipewire-0.3)
endif()

# Add recording sources
set(LMMS_SRCS
    ${LMMS_SRCS}
    src/core/recording/AudioInputAlsa.cpp
    src/core/recording/AudioInputPipeWire.cpp
    src/core/recording/RecordingEngine.cpp
    src/core/recording/RecordingTrack.cpp
    src/core/recording/WavWriter.cpp
)

# Link libraries
if(ALSA_FOUND)
    target_link_libraries(lmms PRIVATE ${ALSA_LIBRARIES})
endif()
if(PIPEWIRE_FOUND)
    target_link_libraries(lmms PRIVATE ${PIPEWIRE_LIBRARIES})
endif()
```

### Step 6: Commit Your Changes
```bash
git add .
git commit -m "Add ALSA and PipeWire audio recording support

- Implement AudioInputDevice abstract base class
- Add ALSA backend for direct hardware access
- Add PipeWire backend for modern audio systems
- Create RecordingEngine for centralized control
- Add multi-track recording support
- Implement thread-safe WAV file writer
- Include example command-line recorder

This expands LMMS recording capabilities beyond JACK-only
to support all major Linux audio systems."
```

### Step 7: Push to Your Fork
```bash
git push origin feature/audio-recording-alsa-pipewire
```

### Step 8: Create Pull Request

1. Go to https://github.com/LMMS/lmms
2. Click "New Pull Request"
3. Click "compare across forks"
4. Select your fork and branch
5. **Title**: `Add ALSA and PipeWire audio recording support`
6. **Description**: Copy content from PR_DESCRIPTION.md
7. Click "Create Pull Request"

## What Happens Next?

1. **Automated Tests**: CI will build and test your code
2. **Code Review**: LMMS maintainers will review
3. **Feedback**: Address any requested changes
4. **Merge**: Once approved, it gets merged!

## Important Tips

### Before Submitting
- [ ] Test on your system first
- [ ] Check code compiles without warnings
- [ ] Run existing LMMS tests (if any)
- [ ] Review the PR description for accuracy

### During Review
- Respond to feedback promptly
- Make requested changes in new commits
- Don't force-push unless asked
- Be patient - reviews take time

### Communication
- Join LMMS Discord: https://lmms.io/chat
- Post in forum: https://lmms.io/forum
- Check existing recording issues: https://github.com/LMMS/lmms/issues?q=recording

## Need Help?

If you get stuck:
1. Read LMMS contributing guide: https://github.com/LMMS/lmms/blob/master/CONTRIBUTING.md
2. Ask in LMMS Discord #development channel
3. Comment on your PR with questions
4. Check similar merged PRs for examples

## Your Code Location

Your clean source files are at:
```
Documents/lmms-audio-recorder-clean/
├── include/     (6 header files)
├── src/         (5 implementation files)
└── examples/    (1 example file)
```

Your PR description is at:
```
Documents/lmms-audio-recorder-clean/PR_DESCRIPTION.md
```

---

**Ready? Start with Step 1: Fork the repo!** 🚀

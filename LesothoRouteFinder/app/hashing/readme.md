# Hashing Visualizer

A native Win32 application for exploring separate chaining (open hashing) and linear probing (closed hashing).

## Build and launch

From this directory, run `build.bat` in a MinGW-w64 UCRT64 environment. The script writes `HashingVisualizer.exe`; launch that executable after the build succeeds. To choose another output name (useful when the app is already open), pass it as the first argument, for example `build.bat HashingVisualizer-check.exe`.

The build script places the matching UCRT64 runtime first and excludes unrelated tool runtimes from the compiler process path. This prevents another application from supplying an incompatible `libwinpthread-1.dll` to GCC's `cc1plus.exe`.

## Workflow

1. Choose **Explore Open Hashing** or **Explore Closed Hashing** on the welcome screen.
2. Enter an integer and choose **Insert value**. Search and Remove accept the same input.
3. Follow the guided steps in the workspace. Use **Pause**, **Resume**, **Step**, and the speed slider to control playback.
4. Choose **More actions** to reveal **Clear** and **Save snapshot**. **Back** returns to method selection.

Each mode maintains its own table state. Open hashing stores colliding values in a bucket's custom linked list. Closed hashing checks successive slots, wraps at the end, leaves deleted markers so searches continue correctly, and grows to a larger prime capacity above a 0.5 load factor.

Operation messages and saved table snapshots append to `hash_results.txt` in the application's current working directory.

## Regression test

Build and run the data-structure regression test from this directory. The script isolates GCC's runtime path the same way as the app build:

```powershell
tests\run_tests.bat
```

The test covers empty-list operations, insertion, duplicate rejection, removals, copy independence and self-assignment; open-hash collisions and negative values; and closed-hash probing, deleted-slot searches, deleted-slot reuse, growth, value preservation, and load factor.

## Verification and limitations

The project uses Win32 controls and GDI, so visual interaction, resizing, minimize/restore, mode navigation, and closing during animation need a Windows desktop run. The `clock_gettime64` compiler error was caused by Windows loading SWI-Prolog's older `C:\Program Files\swipl\bin\libwinpthread-1.dll` for GCC. `build.bat` now isolates the compiler runtime path. A fresh alternate-output GUI build completed successfully, and the data-structure regression test compiled and exited with code 0.

The application currently performs each data-structure mutation when the operation starts and then plays its recorded explanation events; the table therefore reflects the final mutation while the explanation advances. Rehash events currently summarize the resize rather than animating every moved value. These remain follow-up work for a fully synchronized teaching animation. A crash under a particular user sequence was not reproduced in this session; the animation/table mismatch and the ability to clear or change modes during playback were confirmed by source inspection and guarded in the updated UI.

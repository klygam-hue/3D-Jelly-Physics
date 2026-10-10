"""Build and package the native, unsigned iPad 1.9.3 app on macOS/Xcode."""
import argparse
import hashlib
import platform
import plistlib
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path


def run(*args):
    subprocess.run([str(arg) for arg in args], check=True)


def build():
    root = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=root / 'build/ipad-device')
    parser.add_argument('--output', type=Path, default=root / 'package')
    parser.add_argument('--parallel', type=int, default=2)
    args = parser.parse_args()
    if args.parallel < 1:
        parser.error('--parallel must be positive')
    if platform.system() != 'Darwin':
        parser.error('IPA compilation requires macOS with Xcode and the iPhoneOS SDK. No IPA was created.')
    for tool in ('cmake', 'xcrun', 'xcodebuild', 'git'):
        if not shutil.which(tool):
            parser.error(f'Missing {tool}; install Xcode, CMake 3.25+ and Git.')
    run('xcodebuild', '-version')
    run('xcrun', '--sdk', 'iphoneos', '--show-sdk-path')
    directory = args.build_dir.resolve()
    run('cmake', '-S', root, '-B', directory, '-G', 'Xcode',
        '-DCMAKE_SYSTEM_NAME=iOS', '-DCMAKE_OSX_SYSROOT=iphoneos',
        '-DCMAKE_OSX_ARCHITECTURES=arm64', '-DCMAKE_OSX_DEPLOYMENT_TARGET=16.0')
    run('cmake', '--build', directory, '--config', 'Release',
        '--parallel', args.parallel, '--', 'CODE_SIGNING_ALLOWED=NO')
    app = directory / 'Release-iphoneos/3D_Jelly_Physics_Mobile.app'
    with (app / 'Info.plist').open('rb') as stream:
        info = plistlib.load(stream)
    expected = {'CFBundleIdentifier': 'org.jely.mobile',
                'CFBundleShortVersionString': '1.9.3',
                'CFBundleVersion': '193', 'UIDeviceFamily': [2]}
    for key, value in expected.items():
        if info.get(key) != value:
            raise RuntimeError(f'Unexpected device app metadata: {key}={info.get(key)!r}')
    if info.get('DTPlatformName') != 'iphoneos':
        raise RuntimeError('The app must be built for iPhoneOS devices, not a simulator.')
    binary = app / info['CFBundleExecutable']
    archs = subprocess.check_output(['xcrun', 'lipo', '-archs', str(binary)], text=True).split()
    if archs != ['arm64']:
        raise RuntimeError(f'Unexpected device executable architectures: {archs}')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    final = output / '3D-Jelly-Physics-1.9.3-ipad-unsigned.ipa'
    with tempfile.TemporaryDirectory(prefix='ipad-package-', dir=directory) as staging:
        staged = Path(staging) / final.name
        with zipfile.ZipFile(staged, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
            for path in sorted(app.rglob('*')):
                if path.is_file():
                    archive.write(path, (Path('Payload') / app.name / path.relative_to(app)).as_posix())
        with zipfile.ZipFile(staged) as archive:
            if archive.testzip() is not None:
                raise RuntimeError('IPA archive integrity check failed')
        staged.replace(final)
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    (output / (final.name + '.sha256')).write_text(f'{digest}  {final.name}\n')
    for name in ('README.md', 'RELEASE_NOTES_1.9.3.md'):
        shutil.copyfile(root / name, output / name)
    print(f'Unsigned device IPA: {final}\nSHA-256: {digest}')
    print('Apple signing and provisioning are required before installation on an iPad.')


if __name__ == '__main__':
    try:
        build()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f'iPad build failed: {error}', file=sys.stderr)
        sys.exit(1)

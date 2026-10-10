"""Build an installable, test-signed APK with the official Android SDK/NDK."""
import argparse
import os
import shutil
import subprocess
import zipfile
from pathlib import Path
from make_icons import generate


def run(*args, cwd=None):
    subprocess.run([str(a) for a in args],cwd=cwd,check=True)


def build():
    parser=argparse.ArgumentParser()
    parser.add_argument('--sdk',default=os.environ.get('ANDROID_SDK_ROOT') or os.environ.get('ANDROID_HOME'))
    parser.add_argument('--ndk-version',default='28.2.13676358')
    parser.add_argument('--abis',nargs='+',default=['arm64-v8a','x86_64'])
    parser.add_argument('--output',default='Mobile/package')
    args=parser.parse_args()
    if not args.sdk:
        parser.error('Set ANDROID_SDK_ROOT or pass --sdk')
    root=Path(__file__).resolve().parent
    sdk=Path(args.sdk).resolve();ndk=sdk/'ndk'/args.ndk_version
    tools=sdk/'build-tools'/'35.0.0';platform=sdk/'platforms/android-35/android.jar'
    build=root/'build/android';build.mkdir(parents=True,exist_ok=True)
    output=Path(args.output).resolve();output.mkdir(parents=True,exist_ok=True)
    for abi in args.abis:
        directory=build/abi
        run('cmake','-S',root,'-B',directory,'-G','Ninja','-DCMAKE_BUILD_TYPE=Release',f'-DCMAKE_TOOLCHAIN_FILE={ndk}/build/cmake/android.toolchain.cmake',f'-DANDROID_ABI={abi}','-DANDROID_PLATFORM=android-26','-DANDROID_STL=c++_static')
        run('cmake','--build',directory,'--parallel','2')
    generate(build/'icons')
    assets=build/'assets/licenses';assets.mkdir(parents=True,exist_ok=True)
    for source,name in [(root/'../Code/LICENSE','MIT.txt'),(root/'../Code/assets/Inter-OFL.txt','Inter-OFL.txt'),(root/'../Code/vendor/raylib-5.5/LICENSE','raylib.txt')]:
        shutil.copyfile(source,assets/name)
    unsigned=build/'unsigned.apk'
    run(tools/'aapt','package','-f','-M',root/'AndroidManifest.xml','-S',build/'icons/res','-A',build/'assets','-I',platform,'-F',unsigned)
    with zipfile.ZipFile(unsigned,'a',compression=zipfile.ZIP_STORED) as apk:
        prebuilt=next((ndk/'toolchains/llvm/prebuilt').iterdir())
        strip=prebuilt/'bin'/('llvm-strip.exe' if os.name=='nt' else 'llvm-strip')
        for abi in args.abis:
            packaged=build/'package-libs'/abi/'lib3D_Jelly_Physics_Mobile.so'
            packaged.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(build/abi/'lib3D_Jelly_Physics_Mobile.so',packaged)
            run(strip,'--strip-unneeded',packaged)
            apk.write(packaged,f'lib/{abi}/lib3D_Jelly_Physics_Mobile.so')
    aligned=build/'aligned.apk'
    run(tools/'zipalign','-f','-P','16','4',unsigned,aligned)
    keystore=build/'mobile-test.keystore'
    if not keystore.exists():
        run('keytool','-genkeypair','-keystore',keystore,'-storepass','android','-keypass','android','-alias','jely-test','-dname','CN=3D Jelly Physics Mobile Test','-keyalg','RSA','-keysize','2048','-validity','3650')
    final=output/'3D-Jelly-Physics-1.9.2-android-test.apk'
    run(tools/'apksigner','sign','--ks',keystore,'--ks-key-alias','jely-test','--ks-pass','pass:android','--key-pass','pass:android','--out',final,aligned)
    run(tools/'apksigner','verify','--verbose',final)
    run(tools/'zipalign','-c','-P','16','4',final)
    run(tools/'aapt','dump','badging',final)
    shutil.copyfile(root/'README.md',output/'README.md')
    print(final)


if __name__=='__main__':
    build()

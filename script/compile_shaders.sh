#!/bin/bash

INPUT="$1"
OUTPUT="$2"

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
	PLATFORM=linux
	# check for WSL 
	WSL=`uname -r | grep Microsoft`
	if [[ "$WSL" != "" ]]; then
		PLATFORM=win64
		WINDOWS=1
		LINUX=0
	fi
elif [[ "$OSTYPE" == "darwin"* ]]; then
	PLATFORM=macosx
elif [[ "$OSTYPE" == "cygwin" ]] || [[ "$OSTYPE" == "msys" ]]; then
	PLATFORM=win64
fi

HOME=`pwd`

if [[ "$PLATFORM" == "linux" ]]; then
	COMPILER="/media/DATA/Dev/Vulkan/1.2.198.1/x86_64/bin/glslc"
	PACKER="/media/DATA/Dev/Urho3D/Urho3D-Vulkan/tools/SpirvShaderPacker"
elif [[ "$PLATFORM" == "win64" ]]; then
	COMPILER="/mnt/d/DEV/VulkanSdk/1.3.204.1/Bin/glslc.exe"
	PACKER="/mnt/d/DEV/Urho3D/Urho3D-Vulkan/tools/SpirvShaderPacker.exe"	
fi

echo "input=$INPUT"
echo "ouput=$OUTPUT"

mkdir -p $OUTPUT

cd $INPUT

for ext in vert frag
do
	if [[ $ext == "vert" ]]; then
		ext2=vs.spv
		ext3=vs5
	else	
		ext2=ps.spv
		ext3=ps5
	fi
	
	for file in $(ls *.$ext)
	do
		name="${file%.${ext}}" 
		#echo "  compiling $file to $name.$ext2"
		$COMPILER $OPTIONS -o $name.$ext2 $file
		#echo "  packing $name.$ext2 to $name.$ext3"
		$PACKER $name.$ext2
		echo "  generating $name.$ext3"
	done
done
	
mv *.vs5 $OUTPUT
mv *.ps5 $OUTPUT
rm -f *.spv

cd $HOME


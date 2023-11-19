#!/bin/bash

# Gets all shared libs of a file

get_libs() {
    bin_path="$1"

    # Check if $1 is the file's path
    if [[ ! -f "$bin_path" ]]; then
        echo "'$bin_path' is not a file."
        return 1
    fi

    # Check if file is executable
    if [[ ! -x "$bin_path" ]]; then
        echo "'$bin_path' is not an executable file."
        return 1
    fi

    # Traverse through all shared dependencies,
    # Filter shared libraries' paths with grep,
    # Map (awk) and finally get their paths (column 3)
    for lib in $(ldd "$bin_path" | grep '=>' | awk '{print $3}'); do
        echo "$lib"
    done
}

# Create empty lofs.img file
touch lofs.img

# Copies 60 MiB of random data to lofs.img
dd if=/dev/urandom of=lofs.img bs=60M count=1

# Setup loop device 1 on lofs.img
losetup --find --show lofs.img /dev/loop0

# Make file system ext4 on loop device 0
mkfs -t ext4 lofs.img

# Create directory that will represent new FS
mkdir -m 777 lofsdisk

# Mount new FS to the directory
sudo mount -o loop lofs.img lofsdisk

# Compile ex1.c and create executable
sudo gcc ex1.c -o lofsdisk/ex1

# Enter new FS
cd lofsdisk

# Create file1 and give it read-write permission
sudo touch file1
sudo chmod 777 file1

# Create file2 and give it read-write permission
sudo touch file2
sudo chmod 777 file2

# Print content to both files
sudo echo "Arseny" > file1
sudo echo "Savchenko" > file2

# Create directories for libs
sudo mkdir -m 777 bin
sudo mkdir -m 777 lib
sudo mkdir -m 777 lib64

# Copy libs
for cmd in bash cat echo ls; do
    # Copy bin file
    sudo cp "/bin/$cmd" "bin/$cmd"

    # Get lib files
    libs=$(get_libs "/bin/$cmd")

    # Copy each lib file
    for lib in $libs; do
    	sudo cp -v "$lib" "lib/"
    done
done

# Copy 64 bit library
sudo cp -v "/lib64/ld-linux-x86-64.so.2" "lib64/"

# Return to /week11 folder
cd ..

# Change root to lofsdisk
sudo chroot lofsdisk

# These commands are printed in the terminal after chroot
# echo "***** Try 1 *****" > "ex1.txt"
# ./ex1 >> "ex1.txt"

# Copy ex1.txt to the week11 folder
# sudo mv lofsdisk/ex1.txt ex1.txt
# sudo chmod 777 ex1.txt

# These commands are printed on the next try
# echo "***** Try 2 *****" >> "ex1.txt"
# ./ex1 >> "ex1.txt"

# Cleaning
# sudo umount lofsdisk
# sudo rm -r lofsdisk

# Setup basic stuff
cd ~
sudo apt-get upgrade
sudo apt-get update
sudo apt-get install build-essential git python3-pip clang
echo "export CC=/usr/bin/clang" >> ~/.bashrc
echo "export CXX=/usr/bin/clang++" >> ~/.bashrc
source ~/.bashrc

# Install EMP-OT
mkdir OHE
cd OHE
wget https://raw.githubusercontent.com/emp-toolkit/emp-readme/master/scripts/install.py
python3 install.py --install --tool --ot

# Install OHE
git clone https://github.com/anuwu/OHE.git ohe
cd ohe
mkdir build
cd build
cmake ..
make

# Go back to original directory
cd ../..

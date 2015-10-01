sudo add-apt-repository ppa:kalakris/cmake -y;
sudo add-apt-repository ppa:andrewrk/libgroove -y;
if [ $QT5 ]
	then
	sudo add-apt-repository ppa:ubuntu-sdk-team/ppa -y
fi
sudo apt-get update -qq

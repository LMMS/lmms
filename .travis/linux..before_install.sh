sudo add-apt-repository ppa:kalakris/cmake -y;
if [ $QT5 ]
	then
	sudo add-apt-repository ppa:ubuntu-sdk-team/ppa -y
fi
sudo apt-get update -qq

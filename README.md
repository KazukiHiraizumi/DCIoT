# DCIoT
Wireless logging unit for DC reels

## WebBluetoothを使う
  html/dct.htmlがWebBluetoothを使ったWebアプリ版BLEクライアントです。WebBluetoothはソースがhttps://...以外では動作しません。  
secureサーバが無いときでも、localhostであれば動作させる裏技があります。  
html/にてNodejsのhttp-serverにsecureオプションを付け起動
~~~
http-server -S
~~~
証明ファイルは同じくhtml/以下のcert.pem,key.pemという仮のものを使って起動されます。  
さらにChromeのフラグ設定(chrome://flags)で
1. Allow invalid certificates for resources loaded from localhost.  
をEnableにします。これで仮の証明でもパスされます。  
ついでに
2. Experimental Web Platform features  
をEnableにします。これはWebBluetoothを使用可能にします。


## BlueZ 5.43インストール
~~~
sudo  apt-get install debhelper dh-autoreconf flex bison libdbus-glib-1-dev libglib2.0-dev  libcap-ng-dev libudev-dev libreadline-dev libical-dev check dh-systemd libebook1.2-dev devscripts

wget https://launchpad.net/ubuntu/+archive/primary/+files/bluez_5.43.orig.tar.xz
wget https://launchpad.net/ubuntu/+archive/primary/+files/bluez_5.43-0ubuntu1.debian.tar.xz
wget https://launchpad.net/ubuntu/+archive/primary/+files/bluez_5.43-0ubuntu1.dsc

tar xf bluez_5.43.orig.tar.xz
cd bluez-5.43
tar xf ../bluez_5.43-0ubuntu1.debian.tar.xz
debchange --local=~lorenzen 'Backport to Xenial'
debuild -b -j4
cd ..
sudo dpkg -i *.deb
~~~
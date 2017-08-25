installation
============

mac os workstation
------------------

first use the homebrew package manager to install Qt 4.8:

  $ ruby -e "$(curl -fsSL https://raw.github.com/Homebrew/homebrew/go/install)"
  $ brew doctor && brew install qt

then

  $ cd MateBook && make && make install


linux workstation
-----------------

the instance of MateBook compiled for scientific linux 6 depended on lame
3.99.5, yasm 1.2.0, ffmpeg 2.1, cmake 2.8.12, opencv 2.4.11, boost 1.54, zlib
1.2.8, Qt 4.8.6, and phonon 4.7.1.  these dependencies were all built from
scratch as those available through yum were too old.

for scientific linux 7, much newer versions of the dependencies were available
pre-built, albeit not quite as new as those custom built for SL6: cmake 2.8.11,
opencv 2.4.5, boost 1.53, zlib 1.2.7, Qt 4.8.5, phonon 4.6.0.  no repositories
were found for lame 3.99.5, yasm 1.2.0, or ffmpeg 2.1 though, so those are
custom built.

first, install any dependencies not already available as system libraries:

  $ sudo yum install opencv-devel (and others as necessary)

then, build MateBook:

  $ cd MateBook
  $ export PATH=/usr/lib64/qt4/bin/:$PATH
  $ export LD_LIBRARY_PATH=$(pwd)/usr/lib:/usr/local/OpenCV-2.4.5/lib
  $ export JAVA_HOME=/usr/lib/jvm/java-1.7.0-openjdk-1.7.0.111-2.6.7.2.el7_2.x86_64
  $ make && make install  # Qt is installed automatically

note:  i'm not sure what JAVA_HOME is used for.


cluster
-------

first login to a compute node and then:

  $ cd MateBook && make tracker && make installtracker


usage
=====

the exectuable is MateBook/usr/bin/MateBook.  a pre-compiled OS X 10.9
executable is on the labshare at dicksonlab/MateBook/MateBook_mac.

to batch jobs to cluster:

  1. the files must be stored on the labshare
    (i.e. start with /Volumes on a mac, and /tier2 or /groups or /nobackup on linux)

  2. the project must be saved to the labshare

  3. a password-less SSH key pair must be established:

    ssh-keygen -t rsa -P "" -f ~/.ssh/id_rsa
    cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys

  4. the preferences must be configured correctly

    go to matebook menu -> prefs -> system -> cluster processing and set
      transfer host to "login1"
      host key to <blank>
      username to your janelia username (e.g. dicksonb)
      private key to <blank>
      remote environment to "source /misc/lsf/conf/profile.lsf"

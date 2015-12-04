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

  $ cd MateBook
  $ export LD_LIBRARY_PATH=$(pwd)/usr/lib
  $ export JAVA_HOME=/usr/lib/jvm/java-1.7.0-openjdk.x86_64/
  $ make && make install  # Qt is installed automatically


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
    (i.e. start with /Volumes on a mac, and /tier2 on linux)

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
      remote environment to "source /sge/current/default/common/settings.sh"

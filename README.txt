installation
============

for mac and linux use the command-line make utility to install the
dependencies and build MateBook itself.  for windows, good luck, you're on
your own.

gui
---

first use a package manager to install Qt 4.8.  on a mac:

  ruby -e "$(curl -fsSL https://raw.github.com/Homebrew/homebrew/go/install)"
  brew doctor && brew install qt

on fedora linux:

  yum install qt-devel

then clone the latest version of MateBook from the git repository

  git clone /groups/dickson/dicksonlab/MateBook/MateBook.git

to compile MateBook

  cd MateBook && make && make install

to subsequently upgrade to the latest version 

  cd MateBook && git pull && make && make install

tracker
-------

repeat the above clone and Makefile edits on a compute node of the cluster
(skipping the Qt installation) and then

  cd MateBook && make tracker && make installtracker


usage
=====

the exectuable is MateBook/usr/bin/MateBook.  a pre-compiled OS X 10.9
executable is on the labshare at dicksonlab/MateBook/MateBook_mac.

to batch jobs to cluster:

  the files must be stored on the labshare
    (i.e. start with /Volumes on a mac, and /groups on linux)

  the project must be saved to the labshare

  a password-less SSH key pair must be established:

    ssh-keygen -t rsa
      (hit Enter to leave passphrase blank)
    cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys

  the preferences must be configured correctly

    go to matebook menu -> prefs -> system -> cluster processing and set
      transfer host to "login1"
      host key to <blank>
      username to your janelia username (e.g. dicksonb)
      private key to <blank>
      remote environment to "source /sge/current/default/common/settings.sh"

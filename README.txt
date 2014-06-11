installation
============

for mac and linux use the command-line make utility to install the
dependencies and build MateBook itself.  for windows you'll need
to install by hand.


local workstation
-----------------

to build the gui and local tracker:

  $ cd MateBook && make && make install


compute cluster
---------------

first login to a compute node and then:

  $ cd MateBook && make tracker && make installtracker


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

install the dependencies if needed
  on os x:
    ruby -e "$(curl -fsSL https://raw.github.com/Homebrew/homebrew/go/install)"
    brew install cmake ffmpeg boost opencv qt
  on linux:
    yum install cmake ffmpeg boost opencv qt

compile MateBook
  cd MateBook && make gui

establish an SSH key pair with the cluster:
  http://support.apple.com/kb/PH8275

mount the dickson lab share
  finder menu -> go -> connect to server (or just command-K)
  enter smb://dm11/dicksonlab

double-click on dicksonlab/MateBook/MateBook_mac

go to matebook menu -> prefs -> system
under cluster processing set
  transfer host to "login"
  host key to <blank>
  username to your janelia username (e.g. dicksonb)
  private key to <blank>
  remote environment to "source /sge/current/default/common/settings.sh"

to batch jobs to cluster
  the files must be stored on the labshare (i.e. start with /Volumes)
  the project must be saved to the labshare

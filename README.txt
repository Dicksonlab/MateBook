install Qt 4.8.5 <http://qt-project.org/downloads>
  if you haven't already

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

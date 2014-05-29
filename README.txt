compile MateBook
  cd MateBook && make && make install

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

to use the command line utility intead of the gui
  ssh login1
  /groups/dickson/dicksonlab/MateBook/MateBook/usr/bin/tracker/2141/qsub_track.sh "--guiversion" "2141" "--in" "/groups/dickson/dicksonlab/MateBook/457l_ninaEP332.MTS" "--out" "/groups/dickson/dicksonlab/MateBook/test.mbp.mbd/457l_ninaEP332.MTS.1521185265.mbr" "--settings" "settings_2014-05-28T12.59.16.805_1014164536.tsv" "--preprocess" "1" "--track" "0" "--postprocess" "0" "--begin" "0" "--end" "687"

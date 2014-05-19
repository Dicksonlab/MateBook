MB_DIR=/Users/arthurb/src/MateBook

all : gui tracker
gui : $(MB_DIR)/gui/MateBook.app/Contents/MacOS/MateBook
tracker : $(MB_DIR)/gui/MateBook.app/Contents/MacOS/tracker
.PHONY : all gui tracker

cleangui :
	rm -rf $(MB_DIR)/gui/*.o
	rm -rf $(MB_DIR)/gui/moc_*
	rm -rf $(MB_DIR)/gui/MateBook.app/Contents/MacOS/MateBook
cleantracker :
	rm -rf $(MB_DIR)/tracker/build.gcc/tracker
	rm -rf $(MB_DIR)/gui/MateBook.app/Contents/MacOS/tracker
	rm -rf $(MB_DIR)/gui/MateBook.app/Contents/MacOS/*.sh
cleanall : cleangui cleantracker
	rm -rf $(MB_DIR)/gui/MateBook.app
.PHONY : cleangui cleantracker cleanall

$(MB_DIR)/gui/MateBook.app/Contents/MacOS/MateBook : $(MB_DIR)/gui/source/*.cpp $(MB_DIR)/gui/source/*.hpp
	cd gui && ./update.sh && make
	
$(MB_DIR)/gui/MateBook.app/Contents/MacOS/tracker : $(MB_DIR)/tracker/source/*.cpp $(MB_DIR)/tracker/source/*.hpp
	cd $(MB_DIR)/tracker/build.gcc && make
	# bindir=usr/bin/tracker/2141 # on the cluster
	# bindir=$(MB_DIR)/gui/MateBook.app/Contents/MacOS/ # on a mac
	mkdir -p $(MB_DIR)/gui/MateBook.app/Contents/MacOS
	cp $(MB_DIR)/tracker/build.gcc/tracker $(MB_DIR)/gui/MateBook.app/Contents/MacOS
	cp $(MB_DIR)/tracker/source/*.sh $(MB_DIR)/gui/MateBook.app/Contents/MacOS

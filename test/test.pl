#!/usr/bin/perl -w

use strict;

use File::Copy;
use File::Copy "cp";

my $svn = 'svn+ssh://machacek@svn/svn/imp/bioinfo/flyvideo/trunk/';	# url of the repository
my $revision = 1920;	# the first revision to test
my $svnParams = '';	# additional commandline parameters, e.g. '--ignore-externals'
my $cygwin = 'C:\\cygwin\\bin';	# on Windows, a cygwin installation is required for svn and ssh
my $qt = 'C:\\Qt\\4.8.0\\bin\\';	# for copying the Qt .dll files

if ($^O eq 'MSWin32') {
	$ENV{path} .= ';' . $cygwin;
}

$svnParams .= ' --ignore-externals' if ($^O eq 'linux');	# here we will be using the system libs and headers


# pick the lowest revision we have not tried to build here yet
for my $file (glob('*')) {
	if ($file =~ /MateBook_(\d+)_.+/) {
		if ($1 + 1 > $revision) {
			$revision = $1 + 1;
		}
	}
}

for (;;++$revision) {
	# check out
	my $svnError = 0;
	if (!-e 'trunk') {
		print "Checking out revision $revision.\n";
		$svnError ||= system('svn checkout -r ' . $revision . ' ' . $svnParams . ' ' . $svn . ' trunk');
	} else {
		print "Updating to revision $revision.\n";
		$svnError ||= system('svn update -r ' . $revision . ' ' . $svnParams . ' --accept theirs-full ' . $svn . ' trunk');
	}
	if ($svnError) {
		print "SVN checkout or update failed...aborting.\n";
		last;
	}
	
	# patch version in source code
	{
		my $versionFile = 'trunk/gui/source/Version.cpp';
		local $/ = undef;
		open(VERSIONFILE, "<$versionFile") or die $!;
		my $versionFileContent = <VERSIONFILE>;
		close(VERSIONFILE);
		$versionFileContent =~ s/Version::current = \d+;/"Version::current = $revision;"/e;
		open(VERSIONFILE, ">$versionFile") or die $!;
		print VERSIONFILE $versionFileContent;
		close(VERSIONFILE);
	}

	# build
	print "Building for $^O.\n";
	#system('cd trunk && clean.sh');
	my $buildError = 0;
	if ($^O eq 'MSWin32') {
		$buildError ||= system('"C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\devenv.exe" trunk\tracker\build.vs10\tracker.sln /rebuild "Debug|Win32"');
		$buildError ||= !-e 'trunk\tracker\binaries\Win32\Debug\tracker.exe';
		$buildError ||= system('"C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\devenv.exe" trunk\tracker\build.vs10\tracker.sln /rebuild "Release|Win32"');
		$buildError ||= !-e 'trunk\tracker\binaries\Win32\Release\tracker.exe';
		$buildError ||= system('"C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\devenv.exe" trunk\gui\build.vs10\gui.sln /rebuild "Debug|Win32"');
		$buildError ||= !-e 'trunk\gui\binaries\Win32\Debug\MateBook.exe';
		$buildError ||= system('"C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\devenv.exe" trunk\gui\build.vs10\gui.sln /rebuild "Release|Win32"');
		$buildError ||= !-e 'trunk\gui\binaries\Win32\Release\MateBook.exe';
		system('matlab -nosplash -nodesktop -minimize -r "cd \'trunk/flysong\'; compile; exit;" -wait');
	} elsif ($^O eq 'darwin') {
		system('/Applications/MATLAB_R2012a.app/bin/matlab -nosplash -nodesktop -r "cd \'trunk/flysong\'; compile; exit;"');
		$buildError ||= system('xcodebuild -project trunk/gui/build.xc3/MateBook.xcodeproj -configuration Release clean');
		$buildError ||= system('xcodebuild -project trunk/gui/build.xc3/MateBook.xcodeproj -configuration Release build');
		$buildError ||= !-e 'trunk/gui/binaries/OSX/Release/MateBook.app';
	} elsif ($^O eq 'linux') {
		system('matlab -nosplash -nodesktop -r "cd \'trunk/flysong\'; compile; exit;"');
		$buildError = system('cd trunk/tracker/build.gcc && make compile && cd ../../..');
		$buildError ||= !-e 'trunk/tracker/binaries/Linux/Release';
	} else {
		print "I don't know how to build on $^O.\n";
		$buildError = 1;
	}
	if ($buildError) {
		print "Build failed...skipping.\n";
		next;
	}
	print "Build succeeded.\n";

	# test
	print "Testing.\n";
	if ($^O eq 'MSWin32') {
	} elsif ($^O eq 'darwin') {
	} elsif ($^O eq 'linux') {
	} else {
		print "I don't know how to test on $^O.\n";
	}
	
	# package
	my $packageName = 'MateBook_' . $revision . '_' . $^O;
	my $packageFileName = $packageName . '.zip';
	print "Packaging $packageName.\n";
	my $packagingError = 0;
	if ($^O eq 'MSWin32') {
		mkdir($packageName);
		my @trackerFiles = glob('trunk/tracker/binaries/Win32/Release/*');
		my @guiFiles = glob('trunk/gui/binaries/Win32/Release/*');
		my @extraFiles = ($qt.'phonon4.dll', $qt.'QtCLucene4.dll', $qt.'QtCore4.dll', $qt.'QtGui4.dll', $qt.'QtHelp4.dll',  $qt.'QtNetwork4.dll', $qt.'QtOpenGL4.dll', $qt.'QtSql4.dll');
		for my $file (@trackerFiles, @guiFiles, @extraFiles) {
			copy($file, $packageName);
		}
		mkdir($packageName . '/phonon_backend');
		for my $file (glob($qt.'../plugins/phonon_backend/*')) {
			copy($file, $packageName . '/phonon_backend');
		}
		$packagingError ||= system('zip -r ' . $packageFileName . ' ' . $packageName);
	} elsif ($^O eq 'darwin') {
		mkdir($packageName);
		move('trunk/gui/binaries/OSX/Release/MateBook.app', './' . $packageName . '/MateBook.app');
		$packagingError ||= system('zip -r ' . $packageFileName . ' ' . $packageName);
	} elsif ($^O eq 'linux') {
		mkdir($packageName);
		my @trackerFiles = glob('trunk/tracker/binaries/Linux/Release/*');
		for my $file (@trackerFiles) {
			cp($file, $packageName);	# cp preserves permissions in newer versions; copy does not
		}
		for my $file (glob($packageName . '/*')) {
			chmod 0755, $file;
		}
		$packagingError ||= system('zip -r ' . $packageFileName . ' ' . $packageName);
	} else {
		print "I don't know how to package on $^O.\n";
		$buildError = 1;
	}
	if ($packagingError || !-e ($packageName . '.zip')) {
		print "Packaging failed.\n";
		next;
	}
	
	# deploy
	print "Deploying.\n";
	if ($^O eq 'MSWin32') {
		move($packageFileName, '\\\\manray\\dikarchive\\MateBook\\');
	} elsif ($^O eq 'darwin') {
		move($packageFileName, '/Volumes/dikarchive/MateBook/');
	} elsif ($^O eq 'linux') {
		move($packageFileName, '/projects/dikarchive/MateBook/');
		my $targetDir = '/projects/DIK.screen/tracker/' . $revision;
		mkdir($targetDir);
		for my $file (glob($packageName . '/*')) {
			cp($file, $targetDir);	# cp preserves permissions in newer versions; copy does not
		}
		for my $file (glob($targetDir . '/*')) {
			chmod 0755, $file;
		}	
	} else {
		print "I don't know how to deploy on $^O.\n";
	}
}

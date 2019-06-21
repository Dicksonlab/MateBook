function directories = gatherDirectories(baseDirectory, fileName)
	directories = containers.Map;
	
	function gatherInDirs(directory)
		listing = dir(directory);
		for entry = listing'
			if strcmp(entry.name, '.') || strcmp(entry.name, '..')
				continue;
			end

			if isdir([directory '/' entry.name])
				gatherInDirs([directory '/' entry.name]);
			end
		end

		listing = dir([directory '/' fileName]);
		if length(listing) > 1
			disp(['more than one ' fileName ' found in directory ' directory ' ...sanity check failed, returning']);
			return;
		elseif length(listing) == 1
			[videoNameStart videoNameEnd] = regexp(directory, '/[^/]+$');
			if (length(videoNameStart) == 1 || length(videoNameEnd) == 1)
				videoName = directory(videoNameStart(1)+1:videoNameEnd(1));
				if (isempty(videoName))
					disp(['could not determine video name in directory ' directory ' (ending in "/"?)']);
				else
					if isKey(directories, videoName)
						disp(['adding ' directory ' to ' videoName ' group']);
						directories(videoName) = [directories(videoName) {directory}];
					else
						disp(['adding ' directory ' to new ' videoName ' group']);
						directories(videoName) = {directory};
					end
				end
			else
				disp(['could not determine video name in directory ' directory ' (run script from parent directory?)']);
			end
		end
	end

	gatherInDirs(baseDirectory);
end

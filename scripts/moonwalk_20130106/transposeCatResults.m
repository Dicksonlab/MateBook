function transposeCatResults(directory)
	inFile = 'statistics.xlsx';
	inSheet = 'statistics';
	outFile = 'transposeCatResults.xlsx';
	dirPrefix = 'file:///';
	
	if nargin < 1
		inDirs = gatherDirectories('.', inFile);
	elseif nargin < 2
		inDirs = gatherDirectories(directory, inFile);
	end		

	outputLine = 2;
	columnCount = [];
	
	outHandle = xlsopen(outFile);
	for videoName = keys(inDirs)
		videoName = videoName{1};
		inDirGroup = inDirs(videoName);
%		inDirGroup = inDirGroup{1};

		for inDir = inDirGroup
			inDir = inDir{1};
			disp(['processing ' inFile ' in ' inDir]);
			[statistics headers raw] = xlsread([inDir '/' inFile], inSheet);

			headers = {headers{:,1}};
			if isempty(columnCount)
				columnCount = length(headers);
				xlswriteonly(outHandle, headers, 'raw', 'E1');
			elseif columnCount ~= length(headers)	% we could add a more elaborate sanity check that tests if the attributes have the same name
				disp(['attributes in ' inFile ' found in directory ' inDir ' are different from the others, returning']);
				return;
			end

			xlswriteonly(outHandle, {videoName}, 'raw', ['A' num2str(outputLine)]);
			xlswriteonly(outHandle, {[dirPrefix inDir]}, 'raw', ['B' num2str(outputLine)]);
			xlswriteonly(outHandle, raw(:,2:end)', 'raw', ['E' num2str(outputLine)]);

			outputLine = outputLine + size(raw, 2) - 1;
		end
	end
	xlsclose(outHandle);
end

function gatherResults(directory)
	inFile = 'statistics.xlsx';
	inSheet = 'statistics';
	gatheredResultsFile = 'gatheredResults.xlsx';
	dirPrefix = 'file:///';
	
	if nargin < 1
		inDirs = gatherDirectories('.', inFile);
	elseif nargin < 2
		inDirs = gatherDirectories(directory, inFile);
	end		

	outputLine = 2;
	columnCount = [];
	
	gatheredResultsHandle = xlsopen(gatheredResultsFile);
	for videoName = keys(inDirs)
		videoName = videoName{1};
		inDirGroup = inDirs(videoName);
%		inDirGroup = inDirGroup{1};

		for inDir = inDirGroup
			inDir = inDir{1};
			disp(['processing ' inFile ' in ' inDir]);
			[statistics headers] = xlsread([inDir '/' inFile], inSheet);

			headers = {headers{:,1}};
			if isempty(columnCount)
				columnCount = length(headers);
				xlswriteonly(gatheredResultsHandle, headers, 'median', 'E1');
				xlswriteonly(gatheredResultsHandle, headers, 'mean', 'E1');
				xlswriteonly(gatheredResultsHandle, headers, 'stddev', 'E1');
			elseif columnCount ~= length(headers)	% we could add a more elaborate sanity check that tests if the attributes have the same name
				disp(['attributes in ' inFile ' found in directory ' inDir ' are different from the others, returning']);
				return;
			end

			medianStats = nanmedian(statistics, 2)';
			meanStats = nanmean(statistics, 2)';
			stdStats = nanstd(statistics, 0, 2)';

			xlswriteonly(gatheredResultsHandle, {videoName}, 'median', ['A' num2str(outputLine)]);
			xlswriteonly(gatheredResultsHandle, {videoName}, 'mean', ['A' num2str(outputLine)]);
			xlswriteonly(gatheredResultsHandle, {videoName}, 'stddev', ['A' num2str(outputLine)]);

			xlswriteonly(gatheredResultsHandle, {[dirPrefix inDir]}, 'median', ['B' num2str(outputLine)]);
			xlswriteonly(gatheredResultsHandle, {[dirPrefix inDir]}, 'mean', ['B' num2str(outputLine)]);
			xlswriteonly(gatheredResultsHandle, {[dirPrefix inDir]}, 'stddev', ['B' num2str(outputLine)]);

			xlswriteonly(gatheredResultsHandle, medianStats, 'median', ['F' num2str(outputLine)]);
			xlswriteonly(gatheredResultsHandle, meanStats, 'mean', ['F' num2str(outputLine)]);
			xlswriteonly(gatheredResultsHandle, stdStats, 'stddev', ['F' num2str(outputLine)]);

			outputLine = outputLine + 1;
		end
	end
	xlsclose(gatheredResultsHandle);
end

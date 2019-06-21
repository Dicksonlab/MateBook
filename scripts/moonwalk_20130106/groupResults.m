function groupResults(directory)
	inFile = 'statistics.xlsx';
	inSheet = 'statistics';
	groupedResultsFile = 'groupedResults.xlsx';
	dirPrefix = 'file:///';

	if nargin < 1
		inDirs = gatherDirectories('.', inFile);
	elseif nargin < 2
		inDirs = gatherDirectories(directory, inFile);
	end		

	outputLine = 2;
	columnCount = [];

	groupedResultsHandle = xlsopen(groupedResultsFile);
	for videoName = keys(inDirs)
		videoName = videoName{1};
		inDirGroup = inDirs(videoName);
%		inDirGroup = inDirGroup{1};

		groupStatistics = [];
		groupHeaders = [];

		for inDir = inDirGroup
			inDir = inDir{1};
			disp(['processing ' inFile ' in ' inDir]);
			[statistics headers] = xlsread([inDir '/' inFile], inSheet);

			headers = {headers{:,1}};
			if isempty(columnCount)
				columnCount = length(headers);
				xlswriteonly(groupedResultsHandle, headers, 'median', 'G1');
				xlswriteonly(groupedResultsHandle, headers, 'mean', 'G1');
				xlswriteonly(groupedResultsHandle, headers, 'stddev', 'G1');
				xlswriteonly(groupedResultsHandle, {'files' 'graph' 'video' 'per-fly statistics' '#videos' '#flies'}, 'median', 'B1');
				xlswriteonly(groupedResultsHandle, {'files' 'graph' 'video' 'per-fly statistics' '#videos' '#flies'}, 'mean', 'B1');
				xlswriteonly(groupedResultsHandle, {'files' 'graph' 'video' 'per-fly statistics' '#videos' '#flies'}, 'stddev', 'B1');
			elseif columnCount ~= length(headers)	% we could add a more elaborate sanity check that tests if the attributes have the same name
				disp(['attributes in ' inFile ' found in directory ' inDir ' are different from the others, returning']);
				return;
			end

			% account for optional heading correctness value
			rowDifference = size(statistics, 1) - size(groupStatistics, 1);
			if (rowDifference > 0)
				groupStatistics = [groupStatistics; NaN(rowDifference, size(groupStatistics, 2))];
			elseif (rowDifference < 0)
				statistics = [statistics; NaN(-rowDifference, size(statistics, 2))];
			end

			groupStatistics = [groupStatistics statistics];
		end

		medianStats = nanmedian(groupStatistics, 2)';
		meanStats = nanmean(groupStatistics, 2)';
		stdStats = nanstd(groupStatistics, 0, 2)';

		xlswriteonly(groupedResultsHandle, {videoName}, 'median', ['A' num2str(outputLine)]);
		xlswriteonly(groupedResultsHandle, {videoName}, 'mean', ['A' num2str(outputLine)]);
		xlswriteonly(groupedResultsHandle, {videoName}, 'stddev', ['A' num2str(outputLine)]);

		% pipe-separated links
		separator = '|';
		filesLinks = cellfun(@(s) [dirPrefix s separator], inDirGroup, 'UniformOutput', false);
		filesLinks = [filesLinks{:}];
		graphLinks = cellfun(@(s) [dirPrefix s '/motion.png' separator], inDirGroup, 'UniformOutput', false);
		graphLinks = [graphLinks{:}];
		videoLinks = cellfun(@(s) [dirPrefix s '/video.MTS' separator], inDirGroup, 'UniformOutput', false);
		videoLinks = [videoLinks{:}];
		perflyLinks = cellfun(@(s) [dirPrefix s '/statistics.xlsx' separator], inDirGroup, 'UniformOutput', false);
		perflyLinks = [perflyLinks{:}];

		xlswriteonly(groupedResultsHandle, {filesLinks graphLinks videoLinks perflyLinks}, 'median', ['B' num2str(outputLine)]);
		xlswriteonly(groupedResultsHandle, {filesLinks graphLinks videoLinks perflyLinks}, 'mean', ['B' num2str(outputLine)]);
		xlswriteonly(groupedResultsHandle, {filesLinks graphLinks videoLinks perflyLinks}, 'stddev', ['B' num2str(outputLine)]);

		% #videos
		xlswriteonly(groupedResultsHandle, {length(inDirGroup)}, 'median', ['F' num2str(outputLine)]);
		xlswriteonly(groupedResultsHandle, {length(inDirGroup)}, 'mean', ['F' num2str(outputLine)]);
		xlswriteonly(groupedResultsHandle, {length(inDirGroup)}, 'stddev', ['F' num2str(outputLine)]);

		% #flies
		xlswriteonly(groupedResultsHandle, {size(groupStatistics, 2)}, 'median', ['G' num2str(outputLine)]);
		xlswriteonly(groupedResultsHandle, {size(groupStatistics, 2)}, 'mean', ['G' num2str(outputLine)]);
		xlswriteonly(groupedResultsHandle, {size(groupStatistics, 2)}, 'stddev', ['G' num2str(outputLine)]);

		xlswriteonly(groupedResultsHandle, medianStats, 'median', ['H' num2str(outputLine)]);
		xlswriteonly(groupedResultsHandle, meanStats, 'mean', ['H' num2str(outputLine)]);
		xlswriteonly(groupedResultsHandle, stdStats, 'stddev', ['H' num2str(outputLine)]);

		outputLine = outputLine + 1;
	end
	xlsclose(groupedResultsHandle);
end

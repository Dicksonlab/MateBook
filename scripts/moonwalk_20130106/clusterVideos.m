function clusterVideos(gatheredResultsFile)
	outputFile = [gatheredResultsFile '_clustered.xlsx'];
	maxNodes = 10;	% the number of cluster nodes to generate
	attributes = {
		'time in BL|BR',...
		'backward/total in periphery',...
		'total movement',...
		'(#turns forced + #flips forced) / #transitions forced',...
		'#reversals forced / (#turns forced + #reversals forced + #flips forced)',...
		'#stalls in periphery',...
		'#forward turns',...
		'#forward flips',...
		'#stalls forced / #stalls',...
		'number of heading changes',...
	};
	[stats text] = xlsread(gatheredResultsFile, 'median');
	headers = {text{1,6:end}};	% skip hyperlink columns at the beginning
	attributeCount = length(headers);
	assert(attributeCount == size(stats, 2));
	videoCount = size(stats, 1);
	
	stats(isnan(stats)) = 0;

	statsForClustering = stats(:,ismember(headers, attributes));
	assert(size(statsForClustering, 2) == length(attributes));
	%distances = pdist(stats, 'correlation');
	tree = linkage(statsForClustering, 'average', 'correlation');
	figure;
	[~, nodeNumbers] = dendrogram(tree, maxNodes);
	copyfile(gatheredResultsFile, outputFile);

	nodeColumnHeader = 'cluster node';
	nodeColumnName = getExcelCol(size(text, 2) + 1);
	xlswrite(outputFile, {nodeColumnHeader}, 'median', [nodeColumnName num2str(1)]);
	xlswrite(outputFile, {nodeColumnHeader}, 'mean', [nodeColumnName num2str(1)]);
	xlswrite(outputFile, {nodeColumnHeader}, 'stddev', [nodeColumnName num2str(1)]);
	xlswrite(outputFile, nodeNumbers, 'median', [nodeColumnName num2str(2)]);
	xlswrite(outputFile, nodeNumbers, 'mean', [nodeColumnName num2str(2)]);
	xlswrite(outputFile, nodeNumbers, 'stddev', [nodeColumnName num2str(2)]);
	
	meanOfMedians = mean(stats, 1);
	stdOfMedians = std(stats, 1);
	
	% write headers
	xlswrite(outputFile, {'cluster node'}, 'nodes', 'A1');
	xlswrite(outputFile, {'number of videos'}, 'nodes', 'B1');
	xlswrite(outputFile, headers, 'nodes', 'C1');

	% write overall mean of medians
	xlswrite(outputFile, {'mean of medians overall'}, 'nodes', 'A2');
	xlswrite(outputFile, videoCount, 'nodes', 'B2');
	xlswrite(outputFile, meanOfMedians, 'nodes', 'C2');

	% write overall stddev of medians
	xlswrite(outputFile, {'stddev of medians overall'}, 'nodes', 'A3');
	xlswrite(outputFile, videoCount, 'nodes', 'B3');
	xlswrite(outputFile, stdOfMedians, 'nodes', 'C3');

	for node = 1 : maxNodes
		xlswrite(outputFile, node, 'nodes', ['A' num2str(node + 3)]);
		thisNodeStats = stats(nodeNumbers == node,:);
		xlswrite(outputFile, size(thisNodeStats, 1), 'nodes', ['B' num2str(node + 3)]);
		thisNodeMeanOfMedians = mean(thisNodeStats, 1);
		score = (thisNodeMeanOfMedians - meanOfMedians) ./ stdOfMedians;
		xlswrite(outputFile, score, 'nodes', ['C' num2str(node + 3)]);
	end
end

function [distances] = clusterAttributes(gatheredResultsFile)
	[stats headers] = xlsread(gatheredResultsFile, 'median');
	headers = {headers{1,6:end-1}};	% skip hyperlink columns at the beginning and the "heading correctness" column at the end
	attributeCount = length(headers);
	assert(attributeCount == size(stats, 2));
	videoCount = size(stats, 1);
	
	% set NaN to 0
	stats(isnan(stats)) = 0;
	
	distances = pdist(stats', 'correlation');
	tree = linkage(stats', 'average', 'correlation');
	figure;
	dendrogram(tree, 0, 'labels', headers, 'orientation', 'left', 'colorthreshold', 0.2);
end

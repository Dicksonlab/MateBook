function [controlMean, controlStd] = getMeanStd(gatheredResultsFile, videoFileName, attributes)
	filePaths = false;
	
	[stats text] = xlsread(gatheredResultsFile, 'mean');
	headers = {text{1,6:end}};	% skip hyperlink columns at the beginning
	assert(length(headers) == size(stats, 2), 'Could not match attribute names to data. The gathered results file format must have changed.');
	if filePaths
		videoFiles = cellfun(@(s) s(length('file:///U:\user\bidaye\moonwalk\results/')+1:end), {text{2:end,2}}, 'UniformOutput', false);
	else
		videoFiles = {text{2:end,1}};
	end

	videoCount = size(stats, 1);
	attributeCount = size(stats, 2);

	stats(isnan(stats)) = 0;

	% remove the attributes we're not interested in
	if ~isempty(attributes)
		assert(all(ismember(attributes, headers)), 'Not all of the requested attributes could be found in the data.');
		stats = stats(:,ismember(headers, attributes));
		headers = headers(ismember(headers, attributes));
		attributeCount = size(stats, 2);
	end

	controlMean = stats(cell2mat(cellfun(@(s) strcmp(s, videoFileName), videoFiles, 'UniformOutput', false))',:);

	[stats text] = xlsread(gatheredResultsFile, 'stddev');
	headers = {text{1,6:end}};	% skip hyperlink columns at the beginning
	assert(length(headers) == size(stats, 2), 'Could not match attribute names to data. The gathered results file format must have changed.');
	if filePaths
		videoFiles = cellfun(@(s) s(length('file:///U:\user\bidaye\moonwalk\results/')+1:end), {text{2:end,2}}, 'UniformOutput', false);
	else
		videoFiles = {text{2:end,1}};
	end

	videoCount = size(stats, 1);
	attributeCount = size(stats, 2);

	stats(isnan(stats)) = 0;

	% remove the attributes we're not interested in
	if ~isempty(attributes)
		assert(all(ismember(attributes, headers)), 'Not all of the requested attributes could be found in the data.');
		stats = stats(:,ismember(headers, attributes));
		headers = headers(ismember(headers, attributes));
		attributeCount = size(stats, 2);
	end

	controlStd = stats(cell2mat(cellfun(@(s) strcmp(s, videoFileName), videoFiles, 'UniformOutput', false))',:);
end

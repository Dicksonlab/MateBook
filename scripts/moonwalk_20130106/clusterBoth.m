function clusterBoth(gatheredResultsFile)
	attributes = {	% leave empty to use all attributes
		'total forward movement',...
		'total backward movement',...
		'total movement',...
		'forward/total',...
		'backward/total',...
		'backward/forward',...
		'time in BL|BR',...
		'time in FL|FR',...
		'(time in BL|BR) / (time in FL|FR|BL|BR)',...
		'(time in FL|FR) / (time in FL|FR|BL|BR)',...
		'#stalls',...
		'#turns',...
		'#flips',...
		'#reversals',...
		'#reversals / (#turns + #reversals)',...
	};
	filePaths = false;
	minFlyCount = 13;	% keep only videos (or groups) for which there are at least this many flies (TODO: gatheredResults doesn't report the number of flies so this option is currently ignored and works for groupedResults only)
	useMeanStd = 'UAS_TNTE_w.MTS';	% take mean and std from this line (set to [] to use the whole dataset)
	minStdFromMean = 0;	% keep only videos where at least one attribute is at least that many standard deviations from the mean
	minNodes = 3;	% don't display if it's fewer than this
	maxNodes = 50;	% don't split if it's this many or fewer
	optimalLeafOrder = true;
	generateSubClustergrams = true;
	
	[stats text] = xlsread(gatheredResultsFile, 'median');
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

	% remove videos (or groups) for which there aren't enough flies
	[~,flyCountColumn] = ismember('#flies', headers);
	if flyCountColumn
		videosToKeep = stats(:,flyCountColumn) >= minFlyCount;
		stats = stats(videosToKeep,:);
		videoFiles = videoFiles(videosToKeep);
		videoCount = size(stats, 1);
	end

	% remove the attributes we're not interested in
	if ~isempty(attributes)
		assert(all(ismember(attributes, headers)), 'Not all of the requested attributes could be found in the data.');
		stats = stats(:,ismember(headers, attributes));
		headers = headers(ismember(headers, attributes));
		attributeCount = size(stats, 2);
	end
	
	% take mean and std either from the entire file or from just one line (using the "mean" and "stddev" sheets in the Excel file)
	if (isempty(useMeanStd))
		controlMean = mean(stats, 1);
		controlStd = std(stats, 0, 1);
	else
		[controlMean, controlStd] = getMeanStd(gatheredResultsFile, useMeanStd, attributes);
	end

	% standardize each column
	stats = (stats - repmat(controlMean, videoCount, 1)) ./ repmat(controlStd, videoCount, 1);
	
	% remove videos that are too close to the mean
	videosToKeep = any(abs(stats) >= minStdFromMean, 2);
	stats = stats(videosToKeep,:);
	videoFiles = videoFiles(videosToKeep);
	videoCount = size(stats, 1);
	
	cg = clustergram(...
		stats,...
...%		'RowLabels', {text{2:end,1}},...
		'RowLabels', videoFiles,...
		'ColumnLabels', headers,...
		'Standardize', 'none',...	% we're doing that manually above
		'OptimalLeafOrder', optimalLeafOrder,...
		'Colormap', jet(256)...
	);

	if generateSubClustergrams
		groupsDone = false(videoCount - 1, 1);
		groupMarkers = [];

		for group = videoCount - 1 : -1 : 1;
			if groupsDone(group)
				disp(['skipping group ' num2str(group) ' which is already part of another group we have plotted']);
				continue;
			end

			groupInfo = clusterGroup(cg, group, 'row', 'InfoOnly', true);
			if minNodes <= length(groupInfo.RowNodeNames) && length(groupInfo.RowNodeNames) <= maxNodes
				subCg = clusterGroup(cg, group, 'row');
				addTitle(subCg, ['Group ' num2str(group) ' (contains ' num2str(length(groupInfo.RowNodeNames)) ' videos)']);
				disp(group); disp(subCg.RowLabels);
				%view(subCg);
				figureHandle = figure('Visible', 'off', 'Position', [0 0 3000 2000]);
				plot(subCg, figureHandle);
				set(figureHandle, 'PaperPositionMode', 'auto');
				print(figureHandle, '-dpng', ['subgroup' num2str(group)], '-r150');

				markerColor = hsv2rgb([(280+rand()*40)/360 rand()/2+0.5 rand()/2+0.5]);	% random magenta
				groupMarkers = [groupMarkers struct('GroupNumber', {group}, 'Annotation', {num2str(group)}, 'Color', {markerColor})];

				subGroups = cellfun(@(s) str2double(s(7:end)), groupInfo.GroupNames(1:end));
				groupsDone(subGroups) = true;
			else
				disp(['skipping group ' num2str(group) ' of size ' num2str(length(groupInfo.RowNodeNames))]);
			end
		end

		set(cg, 'RowGroupMarker', groupMarkers);
		disp(['sub-clustergram coverage is ' num2str(mean(groupsDone))]);
	end
	
	addTitle(cg, ['Overview (contains ' num2str(videoCount) ' videos)']);
	figureHandle = figure('Visible', 'off', 'Position', [0 0 3000 2000]);
	plot(cg, figureHandle);
	set(figureHandle, 'PaperPositionMode', 'auto');
	axesHandle = findobj(figureHandle, 'Type', 'axes');
	assert(length(axesHandle) >= 2);	% we assume that the second one is the one we want
	colorbar('peer', axesHandle(2), 'WestOutside');
	print(figureHandle, '-dpng', ['overview'], '-r150');
end

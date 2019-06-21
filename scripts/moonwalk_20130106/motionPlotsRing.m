function motionPlotsRing(inDirectory, outDirectory)
	% what attributes to load
	frameAttributeInfo = containers.Map;
	frameAttributeInfo('interpolated') = struct('typeConversion', '*uint8', 'componentCount', 1);
	frameAttributeInfo('isOcclusion') = struct('typeConversion', '*uint8', 'componentCount', 1);
	frameAttributeInfo('trackedFrame') = struct('typeConversion', '*uint32', 'componentCount', 1);
	frameAttributeInfo('trackedTime') = struct('typeConversion', '*float', 'componentCount', 1);
	frameAttributeInfo('videoFrame') = struct('typeConversion', '*uint32', 'componentCount', 1);
	frameAttributeInfo('videoTime') = struct('typeConversion', '*float', 'componentCount', 1);
	flyAttributeInfo = containers.Map;
	flyAttributeInfo('bodyAreaEccentricityCorrected') = struct('typeConversion', '*float', 'componentCount', 1);
	flyAttributeInfo('bodyCentroid') = struct('typeConversion', '*float', 'componentCount', 2);
	flyAttributeInfo('bodyOrientation') = struct('typeConversion', '*float', 'componentCount', 1);
	flyAttributeInfo('filteredBodyCentroid') = struct('typeConversion', '*float', 'componentCount', 2);
	flyAttributeInfo('moved') = struct('typeConversion', '*float', 'componentCount', 2);
	flyAttributeInfo('movedAbs') = struct('typeConversion', '*float', 'componentCount', 1);
	flyAttributeInfo('movedDirectionGlobal') = struct('typeConversion', '*float', 'componentCount', 1);
	flyAttributeInfo('movedDirectionLocal') = struct('typeConversion', '*float', 'componentCount', 1);
	pairAttributeInfo = containers.Map;

	% input file names
	inFile = 'video.tsv';
	headingAnnotationFile = 'heading.txt';
	arenaDescriptionFile = 'arena.tsv';

	% output file names
	motionPlotFile = 'motion_print.png';
	statisticsFile = 'statistics.xlsx';

	% video information
	fps = 25;
	arenaWidth = 20;	% mm, the outer diameter
	arenaCircumference = 18 * pi;	% mm, the distance flies need to walk to complete one circle
	
	% formatting
	motionPlotSize = [1280 900];
	fileFontSize = 0;	% the size video file names (and paths) are printed in
	labelFontSize = 18;
	lineThickness = 10;
	figureBackgroundColor = [0.98 0.98 0.98];
	headingColormap = [0 0 0; 0 0 0; 0 0 0] / 255;
	forwardColor = [0.1 0.8 0.5];
	backwardColor = [0.7 0 0.9];
	stateTransparency = 0.5;	% transparency of state polygons, 0.2 looks nice
	plotRelativePosition = false;
	
	% what input to process
	processResultsWithHeadingAnnotationFileOnly = false;
	skipIfResultsExist = false;
	skipIfResultsNewerThan = '09-Mar-2013 22:45:00';	% set to '' to disable
	normalVideoLength = 300;
	skipShorterVideos = true;
	skipArenaIfQualityBelow = 0.8;	% relative amount of frames containing a properly segmented fly required (so that data is written to the statisticsFile)

	% postprocessing options
	medfiltSize = 13;	% the size of the median filter used to smoothen motion states
	movedFastMedfiltSize = 13; % the size of the median filter used to smoothen the movedFast parameter
	removeIslands = 0;	% structuring element size for opening (a state lasting for less than that many frames is removed)
	bridgeGaps = 0;	% structuring element size for closing (interruptions of a state, when lasting for less than that many frames are removed)
	stateThreshold = [-0.5 0.5];	% if the fly moves slower than that (in mm/s) it will be counted as a stall
	stateChangePenalty = 2; % in mm; the t-score we feed into the hofacker when determining the globally best locomotion state assignment

	if nargin < 1
		motionPlotsRing('.', '.');
		return;
	end
	
	listing = dir(inDirectory);
	for entry = listing'
		if strcmp(entry.name, '.') || strcmp(entry.name, '..')
			continue;
		end
		
		if isdir([inDirectory '/' entry.name])
			[success, message, messageId] = mkdir(outDirectory, entry.name);
			if success
				motionPlotsRing([inDirectory '/' entry.name], [outDirectory '/' entry.name]);
			else
				warning(messageId, message);
				disp(['...not decending into directory ' inDirectory '/' entry.name]);
			end
		end
	end
	
	if skipIfResultsExist && exist([outDirectory '/' statisticsFile], 'file') && exist([outDirectory '/' motionPlotFile], 'file')
		disp(['skipping ' inFile ' in ' inDirectory ' because the results exist already']);
		return;
	end

	listing = dir([inDirectory '/' inFile]);
	if length(listing) > 1
		disp(['more than one ' inFile ' found in directory ' inDirectory ' ...sanity check failed, returning']);
		return;
	elseif length(listing) == 1
		if ~isempty(skipIfResultsNewerThan)
			statisticsListing = dir([outDirectory '/' statisticsFile]);
			if length(statisticsListing) > 1
				disp(['more than one ' statisticsFile ' found in directory ' outDirectory ' ...sanity check failed, returning']);
				return;
			elseif length(statisticsListing) == 1 && datenum(statisticsListing(1).date) > datenum(skipIfResultsNewerThan)
				disp(['skipping ' inFile ' in ' inDirectory ' because the existing results are from ' statisticsListing(1).date]);
				return;
			end
		end
		if processResultsWithHeadingAnnotationFileOnly && ~exist([inDirectory '/' headingAnnotationFile], 'file')
			return;
		end
		disp(['processing data in directory ' inDirectory]);
		if exist([outDirectory '/' statisticsFile], 'file')	% since xlswrite does not create a new file, we have to delete the old one
			delete([outDirectory '/' statisticsFile]);
		end
		
		results = loadVideo(frameAttributeInfo, flyAttributeInfo, pairAttributeInfo, inDirectory);
		
		% return if there are no tracking results
		if ~isfield(results.video(1), 'arena') ||...
			isempty(results.video(1).arena) ||...
			~isfield(results.video(1).arena(1), 'frameAttribute') ||...
			isempty(results.video(1).arena(1).frameAttribute) ||...
			~isfield(results.video(1).arena(1).frameAttribute, 'trackedTime') ||...
			isempty(results.video(1).arena(1).frameAttribute.trackedTime)
			disp('skipping it because there are no tracking results');
			return;
		end
		
		frameCount = length(results.video(1).arena(1).frameAttribute.trackedTime);

		if skipShorterVideos && frameCount < normalVideoLength * fps
			disp(['skipping it because it is shorter than ' num2str(normalVideoLength) ' seconds']);
			return;
		end

		flyCount = length(results.video(1).arena);	% flyCount is really the number of processed arenas
		time = results.video(1).arena(1).frameAttribute.trackedTime;	%TODO: make sure this exists
		
		if motionPlotFile
			figure('Visible', 'off');
		else
			figure;
		end
		
		headingAnnotation = [];
		try
			headingAnnotation = parseHeading([inDirectory '/' headingAnnotationFile], frameCount, fps);
		catch e
		end

		% parse arena description files to get the arena size
		arenaWidthPixels = zeros(flyCount, 1);
		arenaHeightPixels = zeros(flyCount, 1);
		for arenaNumber = 1:flyCount
			arenaFile = fopen([inDirectory '/' results.video(1).arena(arenaNumber).directoryName '/' arenaDescriptionFile]);
			while true
				line = fgetl(arenaFile);
				if ~ischar(line), break, end
				[startIndex endIndex] = regexp(line, 'width\t\d+');
				if length(startIndex) ~= 1
					[startIndex endIndex] = regexp(line, 'height\t\d+');
					if length(startIndex) ~= 1
						continue;
					end
					height = str2double(line(startIndex + length('height') + 1:endIndex));
					arenaHeightPixels(arenaNumber) = height;
					continue;
				end
				width = str2double(line(startIndex + length('width') + 1:endIndex));
				arenaWidthPixels(arenaNumber) = width;
			end
			fclose(arenaFile);
		end
		
		for fly = 1 : flyCount;
			% distance traveled
			velocity1D = results.video(1).arena(fly).flyAttribute(1).movedAbs .* cosd(results.video(1).arena(fly).flyAttribute(1).movedDirectionLocal);
			velocity1D = velocity1D * arenaWidth / arenaWidthPixels(fly) * fps;	% in mm/s
			distance1D = cumsum(velocity1D) / fps;	% in mm, divided by 25 since we have 25 measurements per second
			
			% heading clockwise (+1) or counterclockwise (-1)
			arenaCenter = [arenaWidthPixels(fly) / 2, arenaHeightPixels(fly) / 2];
			fromCenterPosition2D = results.video(1).arena(fly).flyAttribute(1).filteredBodyCentroid - repmat(arenaCenter, [frameCount 1]);
			bodyOrientation = results.video(1).arena(fly).flyAttribute(1).bodyOrientation;
			bodyOrientationVector = [cosd(bodyOrientation), sind(bodyOrientation)];
			heading = fromCenterPosition2D(:,1) .* bodyOrientationVector(:,2) - fromCenterPosition2D(:,2) .* bodyOrientationVector(:,1);

			% radial and angular position
			radialPosition = sqrt((fromCenterPosition2D(:,1) .^ 2) + (fromCenterPosition2D(:,2) .^ 2));
			angularPosition = atan2(fromCenterPosition2D(:,2), fromCenterPosition2D(:,1)) + pi;	% within [0,2*pi]
			circumferentialPosition = angularPosition * arenaCircumference / (2 * pi);
			
%			position = results.video(1).arena(fly).flyAttribute(1).filteredBodyCentroid(:,1) * arenaWidth / arenaWidthPixels(fly);	% in mm
%			heading = (results.video(1).arena(fly).flyAttribute(1).bodyOrientation < 90 | results.video(1).arena(fly).flyAttribute(1).bodyOrientation > 270) * 2 - 1;
%			velocity = results.video(1).arena(fly).flyAttribute(1).moved(:,1) * arenaWidth / arenaWidthPixels(fly) * fps;	% in mm/s
			bodySize = results.video(1).arena(fly).flyAttribute(1).bodyAreaEccentricityCorrected * (arenaWidth / arenaWidthPixels(fly))^2;	% in mm^2

			if length(stateChangePenalty) > 1
				velocity1Dd = double(velocity1D);	% better use double precision here because hofacker sums up a lot of small values
				state = statePartition(velocity1Dd, stateThreshold, stateChangePenalty * fps);
				movedForward = (state' == 3);
				movedBackward = (state' == 1);
			elseif length(stateChangePenalty) == 1
				velocity1Dd = double(velocity1D);	% better use double precision here because hofacker sums up a lot of small values
				s = hofacker2(velocity1Dd', [0 stateChangePenalty * fps * ones(1, length(velocity1D) - 1) 0]);
				movedForward = (s' == velocity1Dd);
				movedBackward = ~movedForward;
			else
				movedForward = velocity1D > 0;
				movedBackward = velocity1D < 0;
%				movedFast = abs(velocity1D) > stallThreshold;

				movedForward = medfilt2_clamp(movedForward, [medfiltSize 1]);
				movedBackward = medfilt2_clamp(movedBackward, [medfiltSize 1]);
%				movedFast = medfilt2_clamp(movedFast, [movedFastMedfiltSize 1]);
			end
			
			if removeIslands
				openStrel = strel('line', removeIslands, 90);
				movedForward = imopen(movedForward, openStrel);
				movedBackward = imopen(movedBackward, openStrel);
			end
			
			if bridgeGaps
				closeStrel = strel('line', bridgeGaps, 90);
				movedForward = imclose(movedForward, closeStrel);
				movedBackward = imclose(movedBackward, closeStrel);
			end
			
			forwardBegin = movedForward & ~[false; movedForward(1:end-1)];
			forwardEnd = movedForward & ~[movedForward(2:end); false];
			backwardBegin = movedBackward & ~[false; movedBackward(1:end-1)];
			backwardEnd = movedBackward & ~[movedBackward(2:end); false];
			
			forwardBeginTime = time(forwardBegin);
			forwardEndTime = time(forwardEnd);
			backwardBeginTime = time(backwardBegin);
			backwardEndTime = time(backwardEnd);
			
			forwardBeginDistance = distance1D(forwardBegin);
			forwardEndDistance = distance1D(forwardEnd);
			backwardBeginDistance = distance1D(backwardBegin);
			backwardEndDistance = distance1D(backwardEnd);

			minDistance = min(distance1D);
			maxDistance = max(distance1D);

			if plotRelativePosition
				subplotHorizontalMargin = 0.05;
				subplotVerticalMargin = 0.08;
				subplotSpacing = 0.01;
				subplotWidth = 1 - 2 * subplotHorizontalMargin;
				subplotHeight = (1 - 2 * subplotVerticalMargin - (flyCount - 1) * subplotSpacing) / flyCount;
				subplotBottom = subplotVerticalMargin + (fly - 1) * (subplotSpacing + subplotHeight);
				subplotLeft = subplotHorizontalMargin;
				axes('Position', [subplotLeft, subplotBottom, subplotWidth, subplotHeight]);
				hold on;
			else
				subplotHorizontalMargin = 0.05;
				subplotVerticalMargin = 0.08;
				subplotSpacing = 0.01;
				subplotWidth = (1 - 2 * subplotHorizontalMargin - (flyCount - 1) * subplotSpacing) / flyCount;
				subplotHeight = 1 - 2 * subplotVerticalMargin;
				subplotBottom = subplotVerticalMargin;
				subplotLeft = subplotHorizontalMargin + (fly - 1) * (subplotSpacing + subplotWidth);
				axes('YDir', 'reverse', 'Position', [subplotLeft, subplotBottom, subplotWidth, subplotHeight]);
				hold on;
			end

%% draw states
			if plotRelativePosition
				xdata = [forwardBeginTime'; forwardBeginTime'; forwardEndTime'; forwardEndTime'];
				ydata = repmat([maxDistance; minDistance; minDistance; maxDistance], [1 length(forwardBeginTime)]);
				zdata = -ones(4, length(forwardBeginTime));
				p = patch(xdata, ydata, zdata, forwardColor, 'EdgeColor', 'none');
				alpha(p, stateTransparency);	% why does this affect the scatter plot as well? MATLAB bug?
				
				xdata = [backwardBeginTime'; backwardBeginTime'; backwardEndTime'; backwardEndTime'];
				ydata = repmat([maxDistance; minDistance; minDistance; maxDistance], [1 length(backwardBeginTime)]);
				zdata = -ones(4, length(backwardBeginTime));
				p = patch(xdata, ydata, zdata, backwardColor, 'EdgeColor', 'none');
				alpha(p, stateTransparency);	% why does this affect the scatter plot as well? MATLAB bug?
			else
				xdata = repmat([arenaCircumference; 0; 0; arenaCircumference], [1 length(forwardBeginTime)]);
				ydata = [forwardBeginTime'; forwardBeginTime'; forwardEndTime'; forwardEndTime'];
				zdata = -ones(4, length(forwardBeginTime));
				p = patch(xdata, ydata, zdata, forwardColor, 'EdgeColor', 'none');
				alpha(p, stateTransparency);	% why does this affect the scatter plot as well? MATLAB bug?
				
				xdata = repmat([arenaCircumference; 0; 0; arenaCircumference], [1 length(backwardBeginTime)]);
				ydata = [backwardBeginTime'; backwardBeginTime'; backwardEndTime'; backwardEndTime'];
				zdata = -ones(4, length(backwardBeginTime));
				p = patch(xdata, ydata, zdata, backwardColor, 'EdgeColor', 'none');
				alpha(p, stateTransparency);	% why does this affect the scatter plot as well? MATLAB bug?
			end
			
%% draw position and heading
			if plotRelativePosition
				scatter(time, distance1D, lineThickness, heading, 'filled');
				caxis([-1 1]);
				grid('on');
				box('on');
				axis([0 time(end) minDistance maxDistance+0.01]);	% adding 0.01 since we could get a "Values must be increasing and non-NaN." error otherwise
				set(gca, 'FontSize', labelFontSize);
				title(['Fly ' num2str(fly)]);
				ylabel(results.video(1).arena(fly).directoryName);
				if fly == 1
					%				ylabel('travel distance (mm)');
					xlabel('time (s)');
				end
				if mod(fly, 2) == 0
					set(gca, 'YAxisLocation', 'right');
				end
				%			set(gca, 'XTick', [0 round(width*periphery) round(width*(1-periphery)) width]);
				set(gca, 'XTick', 0:60:time(end));
				if fly ~= 1
					set(gca, 'XTickLabel', []);
				end
				
				if fly == 1 && fileFontSize
					[filePath, fileName, fileExt] = fileparts(inDirectory);
					text(0, double(-time(end) / 20), [fileName fileExt ' (' filePath ')'], 'Interpreter', 'none', 'FontSize', fileFontSize, 'FontWeight', 'bold');
				end
			else
				scatter(circumferentialPosition, time, lineThickness, heading, 'filled');
				caxis([-1 1]);
				grid('on');
				box('on');
				axis([0 arenaCircumference 0 time(end)]);
				set(gca, 'FontSize', labelFontSize);
				title(['Fly ' num2str(fly)]);
%				titleHandle = title(results.video(1).arena(fly).directoryName, 'Interpreter', 'none', 'FontSize', round(3 * labelFontSize / 4));
%				titlePosition = get(titleHandle, 'Position');
%				set(titleHandle, 'Position', [titlePosition(1), titlePosition(2) + 5 * mod(fly, 2) - 0.4, titlePosition(3)]);
				if fly == 1
					xlabel('position (mm)');
					ylabel('time (s)');
				end
				set(gca, 'XTick', [0 floor(arenaCircumference)]);
				set(gca, 'YTick', 0:60:time(end));
				if fly ~= 1
					set(gca, 'XTickLabel', []);
					set(gca, 'YTickLabel', []);
				end
				
				if fly == 1 && fileFontSize
					[filePath, fileName, fileExt] = fileparts(inDirectory);
					text(0, double(-time(end) / 20), [fileName fileExt ' (' filePath ')'], 'Interpreter', 'none', 'FontSize', fileFontSize, 'FontWeight', 'bold');
				end
			end
			
%% statistics
			% from tracking data
			absMoved = abs(velocity1D);
			totalForwardMovement = nansum(absMoved(movedForward)) / 25;	% in mm, divided by 25 since we have 25 measurements per second
			totalBackwardMovement = nansum(absMoved(movedBackward)) / 25;	% in mm, divided by 25 since we have 25 measurements per second
			totalMovement = totalForwardMovement + totalBackwardMovement;
			backwardOverForward = totalBackwardMovement / totalForwardMovement;
			backwardOverTotal = totalBackwardMovement / totalMovement;
			forwardOverTotal = totalForwardMovement / totalMovement;

			numberOfHeadingChanges = nnz(diff(heading));
			
			meanBodySize = nanmean(bodySize);
			
			% from ethogram
			timeInFLFR = nnz(movedForward) / fps;
			timeInBLBR = nnz(movedBackward) / fps;
			timeInFLFRBLBR = timeInFLFR + timeInBLBR;
			timeInBLBROverFLFR = timeInBLBR / timeInFLFR;
			timeInBLBROverFLFRBLBR = timeInBLBR / timeInFLFRBLBR;
			timeInFLFROverFLFRBLBR = timeInFLFR / timeInFLFRBLBR;
			distInFLFR = sum(abs(forwardEndDistance - forwardBeginDistance));
			distInBLBR = sum(abs(backwardEndDistance - backwardBeginDistance));
			distInFLFRBLBR = distInFLFR + distInBLBR;
			distInBLBROverFLFR = distInBLBR / distInFLFR;
			distInFLFROverFLFRBLBR = distInFLFR / distInFLFRBLBR;
			distInBLBROverFLFRBLBR = distInBLBR / distInFLFRBLBR;
			
			numForwardBouts = size(forwardBeginTime, 1);
			numBackwardBouts = size(backwardBeginTime, 1);
			meanForwardBoutDuration = timeInFLFR / numForwardBouts;
			meanBackwardBoutDuration = timeInBLBR / numBackwardBouts;
			meanForwardBoutDistance = mean(forwardEndDistance - forwardBeginDistance);
			meanBackwardBoutDistance = mean(backwardBeginDistance - backwardEndDistance);	% note that for backward: begin > end
			maxForwardBoutDistance = max(forwardEndDistance - forwardBeginDistance);
			maxBackwardBoutDistance = max(backwardBeginDistance - backwardEndDistance);	% note that for backward: begin > end
			
			headingCorrectness = NaN;
			if ~isempty(headingAnnotation) && ~any(isnan(headingAnnotation(:,fly)))
				headingCorrectness = nnz(headingAnnotation(:,fly) == heading) / frameCount;
			end

			goodQuality = (1 - mean(double(results.video(1).arena(fly).frameAttribute.isOcclusion))) > skipArenaIfQualityBelow;
			
			warning off MATLAB:xlswrite:AddSheet;
			xlswrite([outDirectory '/' statisticsFile], {['Fly ' num2str(fly)]}, 'statistics', [getExcelCol(fly+1) '1']);
			allFlyResults = [...
				totalForwardMovement;
				totalBackwardMovement;
				totalMovement;
				backwardOverForward;
				backwardOverTotal;
				forwardOverTotal;
				numberOfHeadingChanges;
				meanBodySize;
				timeInFLFR;
				timeInBLBR;
				timeInFLFRBLBR;
				timeInBLBROverFLFR;
				timeInBLBROverFLFRBLBR;
				timeInFLFROverFLFRBLBR;
				distInFLFR;
				distInBLBR;
				distInFLFRBLBR;
				distInBLBROverFLFR;
				distInBLBROverFLFRBLBR;
				distInFLFROverFLFRBLBR;
				numForwardBouts;
				numBackwardBouts;
				meanForwardBoutDuration;
				meanBackwardBoutDuration;
				meanForwardBoutDistance;
				meanBackwardBoutDistance;
				maxForwardBoutDistance;
				maxBackwardBoutDistance;
				headingCorrectness;
			];
			if goodQuality
				allFlyResultsForExcel = double(allFlyResults);	% because xlswrite generates 65535 for single-precision NaN, but we want an empty cell
				allFlyResultsForExcel(isinf(allFlyResultsForExcel)) = NaN;	% because xlswrite generates 65535 for Inf, but we want an empty cell
				xlswrite([outDirectory '/' statisticsFile], allFlyResultsForExcel, 'statistics', [getExcelCol(fly+1) '2']);
			else
				disp(['...not writing data to ' statisticsFile ' for arena ' num2str(fly) ' as it is below the quality threshold. (empty arena?)']);
			end
		end
		xlswrite([outDirectory '/' statisticsFile], {...
			'total forward movement';
			'total backward movement';
			'total movement';
			'backward/forward';
			'backward/total';
			'forward/total';
			'number of heading changes';
			'mean body size';
			'time in FL|FR';
			'time in BL|BR';
			'time in FL|FR|BL|BR';
			'(time in BL|BR) / (time in FL|FR)';
			'(time in BL|BR) / (time in FL|FR|BL|BR)';
			'(time in FL|FR) / (time in FL|FR|BL|BR)';
			'dist in FL|FR';
			'dist in BL|BR';
			'dist in FL|FR|BL|BR';
			'(dist in BL|BR) / (dist in FL|FR)';
			'(dist in BL|BR) / (dist in FL|FR|BL|BR)';
			'(dist in FL|FR) / (dist in FL|FR|BL|BR)';
			'#forward bouts';
			'#backward bouts';
			'mean forward bout duration';
			'mean backward bout duration';
			'mean forward bout distance';
			'mean backward bout distance';
			'max forward bout distance';
			'max backward bout distance';
			'heading correctness';
		}, 'statistics', 'A2');
		
%		set(gcf, 'Position', [0, 0, 400, 300]);	% seems to have no effect when using "saveas"
		colormap(headingColormap);
		%myaa;
		if motionPlotFile
			set(gcf,...
				'Color', figureBackgroundColor,...
				'Position', [0 0 motionPlotSize],...
				'PaperPositionMode', 'auto',...
				'InvertHardcopy', 'off'...
			);
			print('-dpng', '-r0', [outDirectory '/' motionPlotFile]);
%			saveas(gcf, [outDirectory '/' motionPlotFile]);
		end
	end
end
function motionPlotsMateBook(inDirectory, outDirectory)
	mateBookVersion = 2141;
	
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
	arenaDescriptionFile = 'arena.tsv';	% for MateBook version >= 2067
	arenaDescriptionFileSuffix = '_arena.tsv';	% for MateBook version < 2067

	% output file names
	motionPlotFile = 'motion_print.png';
	statisticsFile = 'statistics.xlsx';

	% video information
	fps = 25;
	videoWidth = 1920;	% pixels
	arenaWidth = 75;	% mm
	
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
	drawTransitions = false;
	
	% what input to process
	processResultsWithHeadingAnnotationFileOnly = false;
	skipIfResultsExist = false;
	skipIfResultsNewerThan = '09-Mar-2013 22:45:00';	% set to '' to disable
	normalVideoLength = 600;
	skipShorterVideos = true;
	skipArenaIfQualityBelow = 0.8;	% relative amount of frames containing a properly segmented fly required (so that data is written to the statisticsFile)

	% postprocessing options
	stallThreshold = 1.5;	% if the fly moves slower than that (in mm/s) it will be counted as a stall
	medfiltSize = 13;	% the size of the median filter used to smoothen motion states
	movedFastMedfiltSize = 13; % the size of the median filter used to smoothen the movedFast parameter
	removeIslands = 49;	% structuring element size for opening
	bridgeGaps = 25;	% structuring element size for closing
	stateChangePenalty = 2; % in mm; the t-score we feed into the hofacker when determining the globally best locomotion state assignment
	transitionDef = {...
		{{'stall' 'o' [0 0 0]}				{'forward turn' 's' [0 1 0]}		{'forward flip' 'o' [0 1 0]}	{'forward reverse' 'o' [1 1 0]}},...
		{{'forward turn' 's' [0 1 0]}		{'stall' 'o' [0 0 0]}				{'forward reverse' 'o' [1 1 0]}	{'forward flip' 'o' [0 1 0]}},...
		{{'backward flip' 'o' [1 0 0]}		{'backward reverse' 'o' [0 1 1]}	{'stall' 'o' [0 0 0]}			{'backward turn' 's' [1 0 0]}},...
		{{'backward reverse' 'o' [0 1 1]}	{'backward flip' 'o' [1 0 0]}		{'backward turn' 's' [1 0 0]}	{'stall' 'o' [0 0 0]}},...
	};
	stateCount = length(transitionDef);
	forwardLeftState = 1;
	forwardRightState = 2;
	backwardLeftState = 3;
	backwardRightState = 4;
	periphery = 1 / 4;	% the part of the image that is defined to be the periphery both on the left and on the right
	outerPeriphery = 1 / 10;	% the part of the image that is defined to be the outer periphery both on the left and on the right
	forced = 3;	% mm the fly body centroid may be away from the border for transitions to be counted as forced

	if nargin < 1
		motionPlotsMateBook('.', '.');
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
				motionPlotsMateBook([inDirectory '/' entry.name], [outDirectory '/' entry.name]);
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
		
		if mateBookVersion < 2067
			arenaResults = loadSingleVideo(frameAttributeInfo, flyAttributeInfo, pairAttributeInfo, inDirectory);
			frameCount = length(arenaResults(1).frameAttribute.trackedTime);	%TODO: make sure this exists
		else
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
		end

		if skipShorterVideos && frameCount < normalVideoLength * fps
			disp(['skipping it because it is shorter than ' num2str(normalVideoLength) ' seconds']);
			return;
		end

		if mateBookVersion < 2067
			flyCount = length(arenaResults);	% flyCount is really the number of processed arenas
			time = arenaResults(1).frameAttribute.trackedTime;	%TODO: make sure this exists
		else
			flyCount = length(results.video(1).arena);	% flyCount is really the number of processed arenas
			time = results.video(1).arena(1).frameAttribute.trackedTime;	%TODO: make sure this exists
		end
		
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

		% parse arena description files to get the arena widths
		if mateBookVersion < 2067
			arenaWidthPixels = repmat(videoWidth, [flyCount 1]);
			arenaFileInfos = dir([inDirectory '/*' arenaDescriptionFileSuffix]);
			allFileNames = {arenaFileInfos.name};
			allArenaNumbers = [];
			for fileNumber = 1:length(allFileNames)
				fileName = allFileNames{fileNumber};
				arenaNumber = str2double(fileName(1:length(fileName) - length(arenaDescriptionFileSuffix)));
				if (isnan(arenaNumber))	% conversion failed
					arenaFileInfos(fileNumber) = [];
				else
					allArenaNumbers = [allArenaNumbers arenaNumber];
				end
			end
			[allArenaNumbers, fileOrder] = sort(allArenaNumbers);
			arenaFileInfos = arenaFileInfos(fileOrder);
			for fileInfoNumber = 1:length(arenaFileInfos)
				arenaFileInfo = arenaFileInfos(fileInfoNumber);
				arenaFile = fopen([inDirectory '/' arenaFileInfo.name]);
				while true
					line = fgetl(arenaFile);
					if ~ischar(line), break, end
					[startIndex endIndex] = regexp(line, 'width\t\d+');
					if length(startIndex) ~= 1
						continue;
					end
					width = str2double(line(startIndex + length('width') + 1:endIndex));
					arenaWidthPixels(allArenaNumbers(fileInfoNumber) + 1) = width;	% +1 because MATLAB starts indexing at 1
				end
				fclose(arenaFile);
			end
		else
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
		end
		
		for fly = 1 : flyCount;
			if mateBookVersion < 2067
				goodQuality = (1 - mean(double(arenaResults(fly).frameAttribute.isOcclusion))) > skipArenaIfQualityBelow;
				position = arenaResults(fly).flyAttribute(1).filteredBodyCentroid(:,1) * arenaWidth / arenaWidthPixels(fly);	% in mm
				heading = (arenaResults(fly).flyAttribute(1).bodyOrientation < 90 | arenaResults(fly).flyAttribute(1).bodyOrientation > 270) * 2 - 1;
				velocity = arenaResults(fly).flyAttribute(1).moved(:,1) * arenaWidth / arenaWidthPixels(fly) * fps;	% in mm/s
				bodySize = arenaResults(fly).flyAttribute(1).bodyAreaEccentricityCorrected * (arenaWidth / arenaWidthPixels(fly))^2;	% in mm^2
			else
				goodQuality = (1 - mean(double(results.video(1).arena(fly).frameAttribute.isOcclusion))) > skipArenaIfQualityBelow;
				position = results.video(1).arena(fly).flyAttribute(1).filteredBodyCentroid(:,1) * arenaWidth / arenaWidthPixels(fly);	% in mm
				heading = (results.video(1).arena(fly).flyAttribute(1).bodyOrientation < 90 | results.video(1).arena(fly).flyAttribute(1).bodyOrientation > 270) * 2 - 1;
				velocity = results.video(1).arena(fly).flyAttribute(1).moved(:,1) * arenaWidth / arenaWidthPixels(fly) * fps;	% in mm/s
				bodySize = results.video(1).arena(fly).flyAttribute(1).bodyAreaEccentricityCorrected * (arenaWidth / arenaWidthPixels(fly))^2;	% in mm^2
			end
			
			movedDirection = sign(velocity);

			if length(stateChangePenalty) == 1
				velocityd = double(velocity);	% better use double precision here because hofacker sums up a lot of small values
				s = hofacker2(velocityd', [0 stateChangePenalty * fps * ones(1, length(velocity) - 1) 0]);
				movedRight = (s' == velocityd);
				movedLeft = ~movedRight;
				movedForward = ((heading == -1) & movedLeft) | ((heading == 1) & movedRight);
				movedBackward = ((heading == 1) & movedLeft) | ((heading == -1) & movedRight);
				movedFast = abs(velocity) > stallThreshold;
				movedFast = medfilt2_clamp(movedFast, [movedFastMedfiltSize 1]);
			else
				movedForward = (heading == movedDirection);
				movedBackward = (heading ~= movedDirection);
				movedLeft = (movedDirection == -1);
				movedRight = (movedDirection == 1);
				movedFast = abs(velocity) > stallThreshold;

				movedForward = medfilt2_clamp(movedForward, [medfiltSize 1]);
				movedBackward = medfilt2_clamp(movedBackward, [medfiltSize 1]);
				movedLeft = medfilt2_clamp(movedLeft, [medfiltSize 1]);
				movedRight = medfilt2_clamp(movedRight, [medfiltSize 1]);
				movedFast = medfilt2_clamp(movedFast, [movedFastMedfiltSize 1]);
			end
			
			forwardLeft = movedForward & movedLeft & movedFast;
			forwardRight = movedForward & movedRight & movedFast;
			backwardLeft = movedBackward & movedLeft & movedFast;
			backwardRight = movedBackward & movedRight & movedFast;

			if removeIslands
				openStrel = strel('line', removeIslands, 90);
				forwardLeft = imopen(forwardLeft, openStrel);
				forwardRight = imopen(forwardRight, openStrel);
				backwardLeft = imopen(backwardLeft, openStrel);
				backwardRight = imopen(backwardRight, openStrel);
			end
			
			if bridgeGaps
				closeStrel = strel('line', bridgeGaps, 90);
				forwardLeft = imclose(forwardLeft, closeStrel);
				forwardRight = imclose(forwardRight, closeStrel);
				backwardLeft = imclose(backwardLeft, closeStrel);
				backwardRight = imclose(backwardRight, closeStrel);
			end
			
			flBegin = forwardLeft & ~[false; forwardLeft(1:end-1)];
			flEnd = forwardLeft & ~[forwardLeft(2:end); false];
			frBegin = forwardRight & ~[false; forwardRight(1:end-1)];
			frEnd = forwardRight & ~[forwardRight(2:end); false];
			blBegin = backwardLeft & ~[false; backwardLeft(1:end-1)];
			blEnd = backwardLeft & ~[backwardLeft(2:end); false];
			brBegin = backwardRight & ~[false; backwardRight(1:end-1)];
			brEnd = backwardRight & ~[backwardRight(2:end); false];
			
			flBeginTime = time(flBegin);
			flEndTime = time(flEnd);
			frBeginTime = time(frBegin);
			frEndTime = time(frEnd);
			blBeginTime = time(blBegin);
			blEndTime = time(blEnd);
			brBeginTime = time(brBegin);
			brEndTime = time(brEnd);

			% these next ones are not ordered by frame
			forwardBeginPosition = [position(flBegin); position(frBegin)];
			forwardEndPosition = [position(flEnd); position(frEnd)];
			backwardBeginPosition = [position(blBegin); position(brBegin)];
			backwardEndPosition = [position(blEnd); position(brEnd)];
			
			subplotHorizontalMargin = 0.05;
			subplotVerticalMargin = 0.08;
			subplotSpacing = 0.01;
			subplotWidth = (1 - 2 * subplotHorizontalMargin - (flyCount - 1) * subplotSpacing) / flyCount;
			subplotHeight = 1 - 2 * subplotVerticalMargin;
			subplotBottom = subplotVerticalMargin;
			subplotLeft = subplotHorizontalMargin + (fly - 1) * (subplotSpacing + subplotWidth);
			axes('YDir', 'reverse', 'Position', [subplotLeft, subplotBottom, subplotWidth, subplotHeight]);
			hold on;

%% draw transition markers
			state = forwardLeft + 2 * forwardRight + 3 * backwardLeft + 4 * backwardRight;
			state = [state (1:length(state))'];	% taking the indexes with us
			stateBegin = state([1; diff(state(:,1))]~=0,:);	% keep only the frames where states begin
			stateEnd = state([diff(state(:,1)); 1]~=0,:);	% keep only the frames where states end
			stateBegin = stateBegin(stateBegin(:,1)~=0,:); % removing the passive state
			stateEnd = stateEnd(stateEnd(:,1)~=0,:); % removing the passive state

			transition = [1 1 1 frameCount];	% for flies that do not enter any state we initialize this with one long stall
			if ~isempty(stateBegin)
				transition = [stateEnd(1:end-1,:) stateBegin(2:end,:)];	% end of prior state to beginning of next state
				if stateBegin(1,2) ~= 1	% if it does not begin with a state
					transition = [...
						[stateBegin(1,1) 1] stateBegin(1,:);...	% make the first transition (where no prior state is known) a stall
						transition;...
					];
				end
				if stateEnd(end,2) ~= frameCount	% if it does not end in a state
					transition = [...
						transition;...
						stateEnd(end,:) [stateEnd(end,1) frameCount];...	% make the last transition (where no next state is known) a stall
					];
				end
			end
			
			if drawTransitions
				for priorState = 1:stateCount
					for nextState = 1:stateCount
						thisTransitionMask = transition(:,1) == priorState & transition(:,3) == nextState;
						thisTransition = round((transition(thisTransitionMask,2) + transition(thisTransitionMask,4)) / 2);	% mark the midpoint of a transition
%						thisTransition = transition(thisTransitionMask,4);	% mark the end of a transition
						plot(position(thisTransition), time(thisTransition),...
							transitionDef{nextState}{priorState}{2},...	% marker type
							'LineWidth', 0.1,...	% also affects the marker edge
							'MarkerEdgeColor', 'k',...
							'MarkerFaceColor', transitionDef{nextState}{priorState}{3},...
							'MarkerSize', 5);
					end
				end
			end
			
%% draw states
			width = arenaWidth;
			xdata = repmat([width; 0; 0; width], [1 length(flBeginTime)]);
			ydata = [flBeginTime'; flBeginTime'; flEndTime'; flEndTime'];
			zdata = -ones(4, length(flBeginTime));
			p = patch(xdata, ydata, zdata, forwardColor, 'EdgeColor', 'none');
			alpha(p, stateTransparency);	% why does this affect the scatter plot as well? MATLAB bug?
			
			xdata = repmat([width; 0; 0; width], [1 length(frBeginTime)]);
			ydata = [frBeginTime'; frBeginTime'; frEndTime'; frEndTime'];
			zdata = -ones(4, length(frBeginTime));
			p = patch(xdata, ydata, zdata, forwardColor, 'EdgeColor', 'none');
			alpha(p, stateTransparency);	% why does this affect the scatter plot as well? MATLAB bug?
			
			xdata = repmat([width; 0; 0; width], [1 length(blBeginTime)]);
			ydata = [blBeginTime'; blBeginTime'; blEndTime'; blEndTime'];
			zdata = -ones(4, length(blBeginTime));
			p = patch(xdata, ydata, zdata, backwardColor, 'EdgeColor', 'none');
			alpha(p, stateTransparency);	% why does this affect the scatter plot as well? MATLAB bug?
			
			xdata = repmat([width; 0; 0; width], [1 length(brBeginTime)]);
			ydata = [brBeginTime'; brBeginTime'; brEndTime'; brEndTime'];
			zdata = -ones(4, length(brBeginTime));
			p = patch(xdata, ydata, zdata, backwardColor, 'EdgeColor', 'none');
			alpha(p, stateTransparency);	% why does this affect the scatter plot as well? MATLAB bug?

%% draw position and heading
			scatter(position, time, lineThickness, heading, 'filled');
			caxis([-1 1]);
			grid('on');
			box('on');
			axis([0 width 0 time(end)])
			set(gca, 'FontSize', labelFontSize);
			title(['Fly ' num2str(fly)]);
			if fly == 1
				xlabel('x coordinate (mm)');
				ylabel('time (s)');
			end
			set(gca, 'XTick', [0 width]);
			set(gca, 'YTick', 0:60:time(end));
			if fly ~= 1
				set(gca, 'XTickLabel', []);
				set(gca, 'YTickLabel', []);
			end
			
			if fly == 1 && fileFontSize
				[filePath, fileName, fileExt] = fileparts(inDirectory);
				text(0, double(-time(end) / 20), [fileName fileExt ' (' filePath ')'], 'Interpreter', 'none', 'FontSize', fileFontSize, 'FontWeight', 'bold');
			end
			
%% statistics
			% from tracking data
			moved = abs(velocity) / fps;
			absMoved = abs(moved);
			totalForwardMovement = nansum(absMoved(movedForward));
			totalBackwardMovement = nansum(absMoved(movedBackward));
			totalMovement = totalForwardMovement + totalBackwardMovement;
			backwardOverForward = totalBackwardMovement / totalForwardMovement;
			backwardOverTotal = totalBackwardMovement / totalMovement;
			forwardOverTotal = totalForwardMovement / totalMovement;

			inPeriphery = position < width * periphery | position > width * (1-periphery);
			absMovedPeriphery = absMoved(inPeriphery);
			movedForwardPeriphery = movedForward(inPeriphery);
			movedBackwardPeriphery = movedBackward(inPeriphery);
			totalForwardMovementPeriphery = nansum(absMovedPeriphery(movedForwardPeriphery));
			totalBackwardMovementPeriphery = nansum(absMovedPeriphery(movedBackwardPeriphery));
			totalMovementPeriphery = totalForwardMovementPeriphery + totalBackwardMovementPeriphery;
			backwardOverForwardPeriphery = totalBackwardMovementPeriphery / totalForwardMovementPeriphery;
			backwardOverTotalPeriphery = totalBackwardMovementPeriphery / totalMovementPeriphery;
			forwardOverTotalPeriphery = totalForwardMovementPeriphery / totalMovementPeriphery;

			inCenter = ~inPeriphery;
			absMovedCenter = absMoved(inCenter);
			movedForwardCenter = movedForward(inCenter);
			movedBackwardCenter = movedBackward(inCenter);
			totalForwardMovementCenter = nansum(absMovedCenter(movedForwardCenter));
			totalBackwardMovementCenter = nansum(absMovedCenter(movedBackwardCenter));
			totalMovementCenter = totalForwardMovementCenter + totalBackwardMovementCenter;
			backwardOverForwardCenter = totalBackwardMovementCenter / totalForwardMovementCenter;
			backwardOverTotalCenter = totalBackwardMovementCenter / totalMovementCenter;
			forwardOverTotalCenter = totalForwardMovementCenter / totalMovementCenter;
			
			timeInPeriphery = nnz(inPeriphery) / fps;

			inOuterPeriphery = position < width * outerPeriphery | position > width * (1-outerPeriphery);
			timeInOuterPeriphery = nnz(inOuterPeriphery) / fps;

			numberOfHeadingChanges = nnz(diff(heading));
			
			meanBodySize = nanmean(bodySize);
			
			inLeftEndZone = position < forced;
			inRightEndZone = position > (width - forced);

			% from ethogram
			timeInFLFR = (nnz(forwardLeft) + nnz(forwardRight)) / fps;
			timeInBLBR = (nnz(backwardLeft) + nnz(backwardRight)) / fps;
			timeInFLFRBLBR = timeInFLFR + timeInBLBR;
			timeInBLBROverFLFR = timeInBLBR / timeInFLFR;
			timeInBLBROverFLFRBLBR = timeInBLBR / timeInFLFRBLBR;
			timeInFLFROverFLFRBLBR = timeInFLFR / timeInFLFRBLBR;
			distInFLFR = sum(abs(forwardEndPosition - forwardBeginPosition));
			distInBLBR = sum(abs(backwardEndPosition - backwardBeginPosition));
			distInFLFRBLBR = distInFLFR + distInBLBR;
			distInBLBROverFLFR = distInBLBR / distInFLFR;
			distInFLFROverFLFRBLBR = distInFLFR / distInFLFRBLBR;
			distInBLBROverFLFRBLBR = distInBLBR / distInFLFRBLBR;
			
			numForwardReversals = 0;
			numBackwardReversals = 0;
			numForwardTurns = 0;
			numBackwardTurns = 0;
			numForwardFlips = 0;
			numBackwardFlips = 0;
			numStalls = 0;
			numForwardReversalsPeriphery = 0;
			numBackwardReversalsPeriphery = 0;
			numForwardTurnsPeriphery = 0;
			numBackwardTurnsPeriphery = 0;
			numForwardFlipsPeriphery = 0;
			numBackwardFlipsPeriphery = 0;
			numStallsPeriphery = 0;
			numForwardReversalsCenter = 0;
			numBackwardReversalsCenter = 0;
			numForwardTurnsCenter = 0;
			numBackwardTurnsCenter = 0;
			numForwardFlipsCenter = 0;
			numBackwardFlipsCenter = 0;
			numStallsCenter = 0;
			numForwardReversalsForced = 0;
			numBackwardReversalsForced = 0;
			numForwardTurnsForced = 0;
			numBackwardTurnsForced = 0;
			numForwardFlipsForced = 0;
			numBackwardFlipsForced = 0;
			numStallsForced = 0;
			reversalDuration = 0;
			turnDuration = 0;
			flipDuration = 0;
			stallDuration = 0;
			stallDurationForced = 0;
			for priorState = 1:stateCount
				for nextState = 1:stateCount
					thisTransitionMask = transition(:,1) == priorState & transition(:,3) == nextState;
					thisTransition = transition(thisTransitionMask,:);
					thisTransitionName = transitionDef{nextState}{priorState}{1};
					midIndexes = round((thisTransition(:,2) + thisTransition(:,4)) / 2);
					if (strcmp(thisTransitionName, 'stall'))
						numStalls = numStalls + size(thisTransition, 1);
						numStallsPeriphery = numStallsPeriphery + nnz(inPeriphery(midIndexes));
						numStallsCenter = numStallsCenter + nnz(inCenter(midIndexes));
						forcedStallLeft = inLeftEndZone(midIndexes) & heading(midIndexes) == -1;
						forcedStallRight = inRightEndZone(midIndexes) & heading(midIndexes) == 1;
						numStallsForced = numStallsForced + nnz(forcedStallLeft) + nnz(forcedStallRight);
						stallDurationForced = stallDurationForced + sum(thisTransition(forcedStallLeft,4) - thisTransition(forcedStallLeft,2));
						stallDurationForced = stallDurationForced + sum(thisTransition(forcedStallRight,4) - thisTransition(forcedStallRight,2));
						stallDuration = stallDuration + sum(thisTransition(:,4) - thisTransition(:,2));
					elseif strcmp(thisTransitionName, 'forward reverse')
						numForwardReversals = numForwardReversals + size(thisTransition, 1);
						numForwardReversalsPeriphery = numForwardReversalsPeriphery + nnz(inPeriphery(midIndexes));
						numForwardReversalsCenter = numForwardReversalsCenter + nnz(inCenter(midIndexes));
						numForwardReversalsForced = numForwardReversalsForced + nnz(inLeftEndZone(midIndexes) | inRightEndZone(midIndexes));
						reversalDuration = reversalDuration + sum(thisTransition(:,4) - thisTransition(:,2));
					elseif strcmp(thisTransitionName, 'backward reverse')
						numBackwardReversals = numBackwardReversals + size(thisTransition, 1);
						numBackwardReversalsPeriphery = numBackwardReversalsPeriphery + nnz(inPeriphery(midIndexes));
						numBackwardReversalsCenter = numBackwardReversalsCenter + nnz(inCenter(midIndexes));
						numBackwardReversalsForced = numBackwardReversalsForced + nnz(inLeftEndZone(midIndexes) | inRightEndZone(midIndexes));
						reversalDuration = reversalDuration + sum(thisTransition(:,4) - thisTransition(:,2));
					elseif strcmp(thisTransitionName, 'forward turn')
						numForwardTurns = numForwardTurns + size(thisTransition, 1);
						numForwardTurnsPeriphery = numForwardTurnsPeriphery + nnz(inPeriphery(midIndexes));
						numForwardTurnsCenter = numForwardTurnsCenter + nnz(inCenter(midIndexes));
						numForwardTurnsForced = numForwardTurnsForced + nnz(inLeftEndZone(midIndexes) | inRightEndZone(midIndexes));
						turnDuration = turnDuration + sum(thisTransition(:,4) - thisTransition(:,2));
					elseif strcmp(thisTransitionName, 'backward turn')
						numBackwardTurns = numBackwardTurns + size(thisTransition, 1);
						numBackwardTurnsPeriphery = numBackwardTurnsPeriphery + nnz(inPeriphery(midIndexes));
						numBackwardTurnsCenter = numBackwardTurnsCenter + nnz(inCenter(midIndexes));
						numBackwardTurnsForced = numBackwardTurnsForced + nnz(inLeftEndZone(midIndexes) | inRightEndZone(midIndexes));
						turnDuration = turnDuration + sum(thisTransition(:,4) - thisTransition(:,2));
					elseif strcmp(thisTransitionName, 'forward flip')
						numForwardFlips = numForwardFlips + size(thisTransition, 1);
						numForwardFlipsPeriphery = numForwardFlipsPeriphery + nnz(inPeriphery(midIndexes));
						numForwardFlipsCenter = numForwardFlipsCenter + nnz(inCenter(midIndexes));
						numForwardFlipsForced = numForwardFlipsForced + nnz(inLeftEndZone(midIndexes) | inRightEndZone(midIndexes));
						flipDuration = flipDuration + sum(thisTransition(:,4) - thisTransition(:,2));
					elseif strcmp(thisTransitionName, 'backward flip')
						numBackwardFlips = numBackwardFlips + size(thisTransition, 1);
						numBackwardFlipsPeriphery = numBackwardFlipsPeriphery + nnz(inPeriphery(midIndexes));
						numBackwardFlipsCenter = numBackwardFlipsCenter + nnz(inCenter(midIndexes));
						numBackwardFlipsForced = numBackwardFlipsForced + nnz(inLeftEndZone(midIndexes) | inRightEndZone(midIndexes));
						flipDuration = flipDuration + sum(thisTransition(:,4) - thisTransition(:,2));
					else
						disp('transition not counted:');
						thisTransition
					end
				end
			end
			numReversals = numForwardReversals + numBackwardReversals;
			numReversalsPeriphery = numForwardReversalsPeriphery + numBackwardReversalsPeriphery;
			numReversalsCenter = numForwardReversalsCenter + numBackwardReversalsCenter;
			numReversalsForced = numForwardReversalsForced + numBackwardReversalsForced;
			numTurns = numForwardTurns + numBackwardTurns;
			numTurnsPeriphery = numForwardTurnsPeriphery + numBackwardTurnsPeriphery;
			numTurnsCenter = numForwardTurnsCenter + numBackwardTurnsCenter;
			numTurnsForced = numForwardTurnsForced + numBackwardTurnsForced;
			numFlips = numForwardFlips + numBackwardFlips;
			numFlipsPeriphery = numForwardFlipsPeriphery + numBackwardFlipsPeriphery;
			numFlipsCenter = numForwardFlipsCenter + numBackwardFlipsCenter;
			numFlipsForced = numForwardFlipsForced + numBackwardFlipsForced;
			numTransitions = size(transition, 1);
			numTransitionsForced = numStallsForced + numReversalsForced + numTurnsForced + numFlipsForced;
			relReversals = numReversals / numTransitions;
			relTurns = numTurns / numTransitions;
			relFlips = numFlips / numTransitions;
			relStalls = numStalls / numTransitions;
			numReversalsOverNumTurnsNumReversals = numReversals / (numTurns + numReversals);
			numReversalsOverNumTurnsNumReversalsForced = numReversalsForced / (numTurnsForced + numReversalsForced);
			numReversalsOverNumTurnsNumReversalsNumFlipsForced = numReversalsForced / (numTurnsForced + numReversalsForced + numFlipsForced);
			numTurnsNumFlipsOverNumTransitionsForced = (numTurnsForced + numFlipsForced) / numTransitionsForced;
			numStallsForcedOverNumStalls = numStallsForced / numStalls;
			reversalDuration = reversalDuration / fps;
			turnDuration = turnDuration / fps;
			flipDuration = flipDuration / fps;
			stallDuration = stallDuration / fps;
			stallDurationForced = stallDurationForced / fps;
			meanReversalDuration = reversalDuration / numReversals;
			meanTurnDuration = turnDuration / numTurns;
			meanFlipDuration = flipDuration / numFlips;
			meanStallDuration = stallDuration / numStalls;
			meanStallDurationForced = stallDurationForced / numStallsForced;
			numForwardBouts = size(flBeginTime, 1) + size(frBeginTime, 1);
			numBackwardBouts = size(blBeginTime, 1) + size(brBeginTime, 1);
			meanForwardBoutDuration = timeInFLFR / numForwardBouts;
			meanBackwardBoutDuration = timeInBLBR / numBackwardBouts;
			stuckness = mean(((position / arenaWidth - 0.5) * 2) .* heading);
			
			headingCorrectness = NaN;
			if ~isempty(headingAnnotation) && ~any(isnan(headingAnnotation(:,fly)))
				headingCorrectness = nnz(headingAnnotation(:,fly) == heading) / frameCount;
			end

			warning off MATLAB:xlswrite:AddSheet;
			xlswrite([outDirectory '/' statisticsFile], {['Fly ' num2str(fly)]}, 'statistics', [getExcelCol(fly+1) '1']);
			allFlyResults = [...
				totalForwardMovement;
				totalBackwardMovement;
				totalMovement;
				backwardOverForward;
				backwardOverTotal;
				forwardOverTotal;
				totalForwardMovementPeriphery;
				totalBackwardMovementPeriphery;
				totalMovementPeriphery;
				backwardOverForwardPeriphery;
				backwardOverTotalPeriphery;
				forwardOverTotalPeriphery;
				totalForwardMovementCenter;
				totalBackwardMovementCenter;
				totalMovementCenter;
				backwardOverForwardCenter;
				backwardOverTotalCenter;
				forwardOverTotalCenter;
				timeInPeriphery;
				timeInOuterPeriphery;
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
				numReversals;
				numForwardReversals;
				numBackwardReversals;
				numReversalsForced;
				numForwardReversalsForced;
				numBackwardReversalsForced;
				numReversalsPeriphery;
				numForwardReversalsPeriphery;
				numBackwardReversalsPeriphery;
				numReversalsCenter;
				numForwardReversalsCenter;
				numBackwardReversalsCenter;
				numTurns;
				numForwardTurns;
				numBackwardTurns;
				numTurnsForced;
				numForwardTurnsForced;
				numBackwardTurnsForced;
				numTurnsPeriphery;
				numForwardTurnsPeriphery;
				numBackwardTurnsPeriphery;
				numTurnsCenter;
				numForwardTurnsCenter;
				numBackwardTurnsCenter;
				numFlips;
				numForwardFlips;
				numBackwardFlips;
				numFlipsForced;
				numForwardFlipsForced;
				numBackwardFlipsForced;
				numFlipsPeriphery;
				numForwardFlipsPeriphery;
				numBackwardFlipsPeriphery;
				numFlipsCenter;
				numForwardFlipsCenter;
				numBackwardFlipsCenter;
				numStalls;
				numStallsForced;
				numStallsPeriphery;
				numStallsCenter;
				relReversals;
				relTurns;
				relFlips;
				relStalls;
				numReversalsOverNumTurnsNumReversals;
				numReversalsOverNumTurnsNumReversalsForced;
				numReversalsOverNumTurnsNumReversalsNumFlipsForced;
				numTurnsNumFlipsOverNumTransitionsForced;
				numStallsForcedOverNumStalls;
				reversalDuration;
				turnDuration;
				flipDuration;
				stallDuration;
				stallDurationForced;
				meanReversalDuration;
				meanTurnDuration;
				meanFlipDuration;
				meanStallDuration;
				meanStallDurationForced;
				numForwardBouts;
				numBackwardBouts;
				meanForwardBoutDuration;
				meanBackwardBoutDuration;
				stuckness;
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
			'total forward movement in periphery';
			'total backward movement in periphery';
			'total movement in periphery';
			'backward/forward in periphery';
			'backward/total in periphery';
			'forward/total in periphery';
			'total forward movement in center';
			'total backward movement in center';
			'total movement in center';
			'backward/forward in center';
			'backward/total in center';
			'forward/total in center';
			'time in periphery';
			'time in outer periphery';
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
			'#reversals';
			'#forward reversals';
			'#backward reversals';
			'#reversals forced';
			'#forward reversals forced';
			'#backward reversals forced';
			'#reversals in periphery';
			'#forward reversals in periphery';
			'#backward reversals in periphery';
			'#reversals in center';
			'#forward reversals in center';
			'#backward reversals in center';
			'#turns';
			'#forward turns';
			'#backward turns';
			'#turns forced';
			'#forward turns forced';
			'#backward turns forced';
			'#turns in periphery';
			'#forward turns in periphery';
			'#backward turns in periphery';
			'#turns in center';
			'#forward turns in center';
			'#backward turns in center';
			'#flips';
			'#forward flips';
			'#backward flips';
			'#flips forced';
			'#forward flips forced';
			'#backward flips forced';
			'#flips in periphery';
			'#forward flips in periphery';
			'#backward flips in periphery';
			'#flips in center';
			'#forward flips in center';
			'#backward flips in center';
			'#stalls';
			'#stalls forced';
			'#stalls in periphery';
			'#stalls in center';
			'#reversals / #transitions';
			'#turns / #transitions';
			'#flips / #transitions';
			'#stalls / #transitions';
			'#reversals / (#turns + #reversals)';
			'#reversals forced / (#turns forced + #reversals forced)';
			'#reversals forced / (#turns forced + #reversals forced + #flips forced)';
			'(#turns forced + #flips forced) / #transitions forced';
			'#stalls forced / #stalls';
			'total reversal duration';
			'total turn duration';
			'total flip duration';
			'total stall duration';
			'total stall duration forced';
			'mean reversal duration';
			'mean turn duration';
			'mean flip duration';
			'mean stall duration';
			'mean stall duration forced';
			'#forward bouts';
			'#backward bouts';
			'mean forward bout duration';
			'mean backward bout duration';
			'stuckness';
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
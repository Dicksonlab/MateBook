function heading = parseHeading(fileName, frameCount, fps)
	file = fopen(fileName);
	heading = [];
	fly = 1;
	while true
		line = fgetl(file);
		if ~ischar(line), break, end
		[startIndex endIndex] = regexp(line, '\d+:[LR]');
		if isempty(startIndex)
			heading = [heading nan(frameCount, 1)];
			continue;
		end
		thisFlyHeading = zeros(frameCount, 1);
		for change = 1 : length(startIndex)
			newHeading = -1;
			if strcmp(line(endIndex(change)), 'R')
				newHeading = 1;
			end
			thisChangeStartIndex = str2double(line(startIndex(change):endIndex(change)-2)) * fps;
			thisChangeEndIndex = frameCount;
			if change ~= length(startIndex)
				thisChangeEndIndex = str2double(line(startIndex(change+1):endIndex(change+1)-2)) * fps;
			end
			if thisChangeStartIndex < 1
				thisChangeStartIndex = 1;
			end
			if thisChangeEndIndex > frameCount
				thisChangeEndIndex = frameCount;
			end
			thisFlyHeading(thisChangeStartIndex:thisChangeEndIndex) = newHeading;
		end
		heading = [heading thisFlyHeading];
		fly = fly + 1;
	end
	fclose(file);
end
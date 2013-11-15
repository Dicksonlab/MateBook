function results = loadProject(frameAttributeInfo, flyAttributeInfo, pairAttributeInfo)
	videoListing = dir('.');
	videoListing = videoListing([videoListing.isdir] == 1);	% keep only directories
	videoListing = videoListing(~(strcmp({videoListing.name}, '.') | strcmp({videoListing.name}, '..')));	% remove . and ..

	results.video = struct([]);
	for video = videoListing'
		results = loadVideo(frameAttributeInfo, flyAttributeInfo, pairAttributeInfo, video.name, results);
	end
end

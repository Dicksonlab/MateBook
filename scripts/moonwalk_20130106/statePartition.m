function states = statePartition(observations, thresholds, penalties)
%STATEPARTITION   Assign states to a sequence of observations.
%   observations...a vector of observations at consecutive points (e.g. in time)
%   thresholds...a sorted list of S-1 thresholds that separate the S states
%   penalties...a list of S-1 penalties, where penalty(i) is the penalty for changing from state i to state i+1

	epsilon = eps(0);	% use this for edges that have weight 0
	stateCount = length(thresholds) + 1;
	observationCount = length(observations);
	edgeCount = stateCount + observationCount * stateCount + (observationCount - 1) * (stateCount .^ 2) + stateCount;
	nodeCount = 1 + 2 * observationCount * stateCount + 1;

	cumPenalties = [0 cumsum(penalties)];
	penaltyMatrix = zeros(stateCount);
	for row = 1:stateCount
		for col = 1:stateCount
			penalty = cumPenalties(abs(col - row) + 1);
			if penalty == 0
				penalty = epsilon;
			end
			penaltyMatrix(row, col) = penalty;
		end
	end

	edgeFrom = zeros(1, edgeCount);
	edgeTo = zeros(1, edgeCount);
	weights = zeros(1, edgeCount);
	
	for s = 1:stateCount
		edgeFrom(s) = 1;
		edgeTo(s) = s + 1;
		weights(s) = epsilon;
	end
	
	baseNode = 1;
	baseEdge = stateCount;
	for o = 1:observationCount
		observation = observations(o);
		for s = 1:stateCount
			edgeFrom(baseEdge + s) = baseNode + s;
			edgeTo(baseEdge + s) = baseNode + stateCount + s;
			if s == 1
				weight = thresholds(1) - observation;
			elseif s == stateCount
				weight = observation - thresholds(end);
			else
				weight = min(thresholds(s) - observation, observation - thresholds(s-1));	% the distance to the closer threshold
			end
			weights(baseEdge + s) = -weight;	% - because we're looking for the shortest path
		end
		baseNode = baseNode + stateCount;
		baseEdge = baseEdge + stateCount;
		if o ~= observationCount
			for sStart = 1:stateCount
				for sEnd = 1:stateCount
					edgeFrom(baseEdge + stateCount * (sEnd - 1) + sStart) = baseNode + sStart;
					edgeTo(baseEdge + stateCount * (sEnd - 1) + sStart) = baseNode + stateCount + sEnd;
					weights(baseEdge + stateCount * (sEnd - 1) + sStart) = penaltyMatrix(sStart, sEnd);
				end
			end
			baseNode = baseNode + stateCount;
			baseEdge = baseEdge + (stateCount .^ 2);
		end
	end

	for s = 1:stateCount
		edgeFrom(edgeCount + 1 - s) = nodeCount - s;
		edgeTo(edgeCount + 1 - s) = nodeCount;
		weights(edgeCount + 1 - s) = epsilon;
	end
	
	graph = sparse(edgeFrom, edgeTo, weights, nodeCount, nodeCount);
	[~, path] = graphshortestpath(graph, 1, nodeCount, 'Method', 'Acyclic');

%% result visualization
% 	h = view(biograph(graph,[],'ShowWeights','on'));
% 	set(h.Nodes(path),'Color',[1 0.4 0.4])
% 	fowEdges = getedgesbynodeid(h,get(h.Nodes(path),'ID'));
% 	revEdges = getedgesbynodeid(h,get(h.Nodes(fliplr(path)),'ID'));
% 	edges = [fowEdges;revEdges];
% 	set(edges,'LineColor',[1 0 0]);
% 	set(edges,'LineWidth',1.5);

%% return one state per observation
	states = path(2:2:end-2);
	states = mod(states - 2, stateCount) + 1;
end
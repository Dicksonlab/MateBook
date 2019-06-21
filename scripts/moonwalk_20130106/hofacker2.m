function [ret_s ret_t flippos confidence]  = hofacker2(s, t)
% Calls dynamic programming routine, that optimizes total probability for
% occlusion and sequence observations. Total probability may be improved by
% a "flip" operation, that inverts (multiply by -1) scores for two occlusions
% and all sequence scores between those occlusions.
% parameters:
%   s: scores for sequences (as probabilities)
%   t: scores for occlusions (as probabilities)
% returns:
%   ret_s, ret_t: s, t after flip operations performed at flip positions.
%   flippos:      contains 1 at flip positions, 0 otherwise
% sample call:
%   [ret_s ret_t flippos] = hofacker2([1 1 1 3 1 1 1], [0 10 10 -2 -2 10 10 0])

%% make a random list
if nargin == 1
    t = s(1 : 2 : end);
    s = s(2 : 2 : end);
end
if isempty(s)
    error 'empty parameter s';
end
if length(s) ~= length(t)-1
    error 'parameter mismatch: s should have length of t-1';
end
%if mod(length(t), 2) ~= 0
%    error 'length of parameter t must be odd';
%end

s = [0 s];
t = [0 t];

n = length(s);


%% DP solution

% S[i][c] holds the best score for the subsequence s[1..i] where the "sign"
% at pos i is c. The Matlab implementation uses 2, resp. si(1) for sign "+" and 1, resp. si(-1) for "-"

S = NaN(n, 2);
T = NaN(n+1, 2);
T(1,si(-1)) = -inf;
T(1,si(+1)) = 0;

for i = 1 : n
    for c = [-1 +1]
        S(i, si(c)) = T(i, si(c)) + c * s(i); % add score if sign positive, subtract if negative
    end
    for c = [-1 +1]
        T(i+1, si(c)) = max(S(i, si(c)) + t(i+1), S(i, si(c * -1)) - t(i+1)); % either add to same score or subtract from flipped score - take whatever is higher
    end
end

[~, bestScoreSign] = max(T(n+1,:));
bestScoreSign = bestScoreSign * 2 - 3;
%fprintf('Best score: %f\n', T(n+1, si(bestScoreSign)));

%% backtracking
flippos = zeros(n+1, 1);
c = bestScoreSign;
for i = n+1 : -1 : 2
%    if T(i, si(c)) == T(i-1, si(c)) + t(i) + s(i-1)
    if T(i, si(c)) == S(i-1, si(c)) + t(i)
        flippos(i) = 0;
    else
        flippos(i) = 1;
        c = c * -1;
    end
end

%% 
s = s(2:end);
t = t(2:end);
flippos = flippos(2:end);
n = n - 1;

%% recompute for testing

%fprintf('recomputed score %f\n', score(flippos));
%greedy_flippos = greedy();
%fprintf(' Greedy score %f\n', score(greedy_flippos));

[ret_s ret_t] = bulk_flip(s, t, flippos);
if nargout > 3
    confidence = compute_confidence(ret_s, ret_t);
end

    function ret = si(x)
    % maps scores +1, -1 to indices 1, 2
        ret = (x+1)/2 + 1;
    end

    function ret = score (fp)
    % computes score of given flippositions
        [flipS flipT] = bulk_flip(s, t, fp);
        ret = sum(flipS) + sum(flipT);
%        fp = ((fp.*2-1) * -1)';
%        ret = sum(fp .* t) + sum(cumprod(fp(1:end-1)) .* s);
    end

    function [s t] = bulk_flip(s, t, flippos)
    % flips values of s and t according to given flip positions in flippos
        fp = ((flippos.*2-1) * -1)';   % returns -1 if flippos was true, +1 otherwise
        t = t .* fp;                   % t := -t at flippositions, remains the same otherwise
        s = s .* cumprod(fp(1:end-1)); % s := -s between flippositions, remains same otherwise
    end
    function confidence = compute_confidence(ret_s, ret_t)
        confidenceM = NaN(length(ret_t));
        for i = 1 : length(ret_t)
            for j = i+1 : length(ret_t)
                v = cat(2, ret_t(i), ret_t(j), ret_s(i:j-1));
                c = sum(v);
                confidenceM(i,j) = c;
                confidenceM(j,i) = c;
            end
        end
        confidence = min(confidenceM, [], 2);
    end


    function fp = greedy
        fp = zeros(n+1, 1);
        bestfp = fp;
        maxS = score(fp);
        found = true;
        while(found)
            found = false;
            for i = 1 : n + 1
                for j = i+1 : n + 1
                    tfp = fp;
                    tfp(i) = ~tfp(i);
                    tfp(j) = ~tfp(j);
                    tS = score(tfp);
                    if tS > maxS
                        bestfp = tfp;
                        maxS = tS;
                        found = true;
                    end
                end
            end
            fp = bestfp;
        end
    end
end
function [success,message]=xlswriteonly(xlshandle,data,sheet,range)
% XLSWRITE Stores numeric array or cell array in Excel workbook.
%   [SUCCESS,MESSAGE]=XLSWRITE(FILE,ARRAY,SHEET,RANGE) writes ARRAY to the Excel
%   workbook, FILE, into the area, RANGE in the worksheet specified in SHEET.
%   FILE and ARRAY must be specified. If either FILE or ARRAY is empty, an
%   error is thrown and XLSWRITE terminates. The first worksheet of the
%   workbook is the default. If SHEET does not exist, a new sheet is added at
%   the end of the worksheet collection. If SHEET is an index larger than the
%   number of worksheets, new sheets are appended until the number of worksheets
%   in the workbook equals SHEET. The size defined  by the  RANGE should fit the
%   size of ARRAY or contain only the first cell, e.g. 'A2'. If RANGE is larger
%   than the size of ARRAY, Excel will fill the remainder of the region with
%   #N/A. If RANGE is smaller than the size of ARRAY, only the sub-array that
%   fits into RANGE will be written to FILE. The success of the operation is
%   returned in SUCCESS and any accompanying message, in MESSAGE. On error,
%   MESSAGE shall be a struct, containing the error message and message ID.
%   See NOTE 1.
%
%   To specify SHEET or RANGE, but not both, you can call XLSWRITE with
%   just three inputs. If the third input is a string that includes a colon 
%   character (e.g., 'D2:H4'), it specifies RANGE. If it is not (e.g., 
%   'SALES'), it specifies the worksheet to write to. See the next two 
%   syntaxes below.
%
%   [SUCCESS,MESSAGE]=XLSWRITE(FILE,ARRAY,SHEET) writes ARRAY to the Excel
%   workbook, FILE, starting at cell A1 and using SHEET as described above.
%
%   [SUCCESS,MESSAGE]=XLSWRITE(FILE,ARRAY,RANGE) writes ARRAY to the Excel
%   workbook, FILE, in the first worksheet and using RANGE as described above.
%
%   [SUCCESS,MESSAGE]=XLSWRITE(FILE,ARRAY) writes ARRAY to the Excel
%   workbook, FILE, starting at cell A1 of the first worksheet. The return
%   values are as for the above example.
%
%   XLSWRITE ARRAY FILE, is the command line version of the above example.
%
%   INPUT PARAMETERS:
%       file:   string defining the workbook file to write to.
%               Default directory is pwd; default extension 'xls'.
%       array:  m x n numeric array or cell array.
%       sheet:  string defining worksheet name;
%               double, defining worksheet index.
%       range:  string defining data region in worksheet, using the Excel
%               'A1' notation.
%
%   RETURN PARAMETERS:
%       SUCCESS: logical scalar.
%       MESSAGE: struct containing message field and message_id field.
%
%   EXAMPLES:
%
%   SUCCESS = XLSWRITE('c:\matlab\work\myworkbook.xls',A,'A2:C4') will write A to
%   the workbook file, myworkbook.xls, and attempt to fit the elements of A into
%   the rectangular worksheet region, A2:C4. On success, SUCCESS will contain true,
%   while on failure, SUCCESS will contain false.
%
%   NOTE 1: The above functionality depends upon Excel as a COM server. In
%   absence of Excel, ARRAY shall be written as a text file in CSV format. In
%   this mode, the SHEET and RANGE arguments shall be ignored.
%
% See also XLSREAD, CSVWRITE.
%

%   Copyright 1984-2008 The MathWorks, Inc.
%   $Revision: 1.1.6.17.4.1 $  $Date: 2010/06/24 19:34:38 $
%==============================================================================
% Set default values.
Sheet1 = 1;

if nargin < 3
    sheet = Sheet1;
    range = '';
elseif nargin < 4
    range = '';
end

if nargout > 0
    success = true;
    message = struct('message',{''},'identifier',{''});
end

% Handle input.
try
    % Check for empty input data
    if isempty(data)
        error('MATLAB:xlswrite:EmptyInput','Input array is empty.');
    end

    % Check for N-D array input data
    if ndims(data)>2
        error('MATLAB:xlswrite:InputDimension',...
            'Dimension of input array cannot be higher than two.');
    end

    % Check class of input data
    if ~(iscell(data) || isnumeric(data) || ischar(data)) && ~islogical(data)
        error('MATLAB:xlswrite:InputClass',...
            'Input data must be a numeric, cell, or logical array.');
    end


    % convert input to cell array of data.
     if iscell(data)
        A=data;
     else
         A=num2cell(data);
     end

    if nargin > 2
        % Verify class of sheet parameter.
        if ~(ischar(sheet) || (isnumeric(sheet) && sheet > 0))
            error('MATLAB:xlswrite:InputClass',...
                'Sheet argument must be a string or a whole number greater than 0.');
        end
        if isempty(sheet)
            sheet = Sheet1;
        end
        % parse REGION into sheet and range.
        % Parse sheet and range strings.
        if ischar(sheet) && ~isempty(strfind(sheet,':'))
            range = sheet; % only range was specified.
            sheet = Sheet1;% Use default sheet.
        elseif ~ischar(range)
            error('MATLAB:xlswrite:InputClass',...
                'Range argument must be a string in Excel A1 notation.');
        end
    end

catch exception
    if ~isempty(nargchk(2,4,nargin))
        error('MATLAB:xlswrite:InputArguments',nargchk(2,4,nargin));
    else
        success = false;
        message = exceptionHandler(nargout, exception);
    end
    return;
end
%------------------------------------------------------------------------------
    Excel = xlshandle.Excel;
%------------------------------------------------------------------------------
try
    % Construct range string
    if isempty(strfind(range,':'))
        % Range was partly specified or not at all. Calculate range.
        [m,n] = size(A);
        range = calcrange(range,m,n);
    end
catch exception
    success = false;
    message = exceptionHandler(nargout, exception);
    return;
end

%------------------------------------------------------------------------------
            try % select region.
                % Activate indicated worksheet.
                message = activate_sheet(Excel,sheet);

                % Select range in worksheet.
                Select(Range(Excel,sprintf('%s',range)));

            catch exceptionInner % Throw data range error.
                throw(MException('MATLAB:xlswrite:SelectDataRange', sprintf('Excel returned: %s.', exceptionInner.message))); 
            end

            % Export data to selected region.
            set(Excel.selection,'Value',A);
end
%--------------------------------------------------------------------------
function message = activate_sheet(Excel,Sheet)
% Activate specified worksheet in workbook.

% Initialise worksheet object
WorkSheets = Excel.sheets;
message = struct('message',{''},'identifier',{''});

% Get name of specified worksheet from workbook
try
    TargetSheet = get(WorkSheets,'item',Sheet);
catch exception  %#ok<NASGU>
    % Worksheet does not exist. Add worksheet.
    TargetSheet = addsheet(WorkSheets,Sheet);
    warning('MATLAB:xlswrite:AddSheet','Added specified worksheet.');
    if nargout > 0
        [message.message,message.identifier] = lastwarn;
    end
end

% activate worksheet
Activate(TargetSheet);
end
%------------------------------------------------------------------------------
function newsheet = addsheet(WorkSheets,Sheet)
% Add new worksheet, Sheet into worsheet collection, WorkSheets.

if isnumeric(Sheet)
    % iteratively add worksheet by index until number of sheets == Sheet.
    while WorkSheets.Count < Sheet
        % find last sheet in worksheet collection
        lastsheet = WorkSheets.Item(WorkSheets.Count);
        newsheet = WorkSheets.Add([],lastsheet);
    end
else
    % add worksheet by name.
    % find last sheet in worksheet collection
    lastsheet = WorkSheets.Item(WorkSheets.Count);
    newsheet = WorkSheets.Add([],lastsheet);
end
% If Sheet is a string, rename new sheet to this string.
if ischar(Sheet)
    set(newsheet,'Name',Sheet);
end
end
%------------------------------------------------------------------------------
function range = calcrange(range,m,n)
% Calculate full target range, in Excel A1 notation, to include array of size
% m x n

range = upper(range);
cols = isletter(range);
rows = ~cols;
% Construct first row.
if ~any(rows)
    firstrow = 1; % Default row.
else
    firstrow = str2double(range(rows)); % from range input.
end
% Construct first column.
if ~any(cols)
    firstcol = 'A'; % Default column.
else
    firstcol = range(cols); % from range input.
end
try
    lastrow = num2str(firstrow+m-1);   % Construct last row as a string.
    firstrow = num2str(firstrow);      % Convert first row to string image.
    lastcol = dec2base27(base27dec(firstcol)+n-1); % Construct last column.

    range = [firstcol firstrow ':' lastcol lastrow]; % Final range string.
catch exception 
    error('MATLAB:xlswrite:CalculateRange', 'Invalid data range: %s.', range);
end
end
%----------------------------------------------------------------------
function string = index_to_string(index, first_in_range, digits)

letters = 'A':'Z';
working_index = index - first_in_range;
outputs = cell(1,digits);
[outputs{1:digits}] = ind2sub(repmat(26,1,digits), working_index);
string = fliplr(letters([outputs{:}]));
end
%----------------------------------------------------------------------
function [digits first_in_range] = calculate_range(num_to_convert)

digits = 1;
first_in_range = 0;
current_sum = 26;
while num_to_convert > current_sum
    digits = digits + 1;
    first_in_range = current_sum;
    current_sum = first_in_range + 26.^digits;
end
end
%------------------------------------------------------------------------------
function s = dec2base27(d)

%   DEC2BASE27(D) returns the representation of D as a string in base 27,
%   expressed as 'A'..'Z', 'AA','AB'...'AZ', and so on. Note, there is no zero
%   digit, so strictly we have hybrid base26, base27 number system.  D must be a
%   negative integer bigger than 0 and smaller than 2^52.
%
%   Examples
%       dec2base(1) returns 'A'
%       dec2base(26) returns 'Z'
%       dec2base(27) returns 'AA'
%-----------------------------------------------------------------------------

d = d(:);
if d ~= floor(d) || any(d(:) < 0) || any(d(:) > 1/eps)
    error('MATLAB:xlswrite:Dec2BaseInput',...
        'D must be an integer, 0 <= D <= 2^52.');
end
[num_digits begin] = calculate_range(d);
s = index_to_string(d, begin, num_digits);
end
%------------------------------------------------------------------------------
function d = base27dec(s)
%   BASE27DEC(S) returns the decimal of string S which represents a number in
%   base 27, expressed as 'A'..'Z', 'AA','AB'...'AZ', and so on. Note, there is
%   no zero so strictly we have hybrid base26, base27 number system.
%
%   Examples
%       base27dec('A') returns 1
%       base27dec('Z') returns 26
%       base27dec('IV') returns 256
%-----------------------------------------------------------------------------

if length(s) == 1
   d = s(1) -'A' + 1;
else
    cumulative = 0;
    for i = 1:numel(s)-1
        cumulative = cumulative + 26.^i;
    end
    indexes_fliped = 1 + s - 'A';
    indexes = fliplr(indexes_fliped);
    indexes_in_cells = mat2cell(indexes, 1, ones(1,numel(indexes)));
    d = cumulative + sub2ind(repmat(26, 1,numel(s)), indexes_in_cells{:});
end
end
%-------------------------------------------------------------------------------

function messageStruct = exceptionHandler(nArgs, exception)
    if nArgs == 0
        throwAsCaller(exception);  	   
    else
        messageStruct.message = exception.message;       
        messageStruct.identifier = exception.identifier;
    end
end

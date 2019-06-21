function xlshandle=xlsopen(file)
% XLSOPEN Opens an Excel workbook or creates it if it doesn't exist.
%==============================================================================
try
    % handle requested Excel workbook filename.
    if ~isempty(file)
        if ~ischar(file)
            error('MATLAB:xlswrite:InputClass','Filename must be a string.');
        end
        % check for wildcards in filename
        if any(findstr('*', file))
            error('MATLAB:xlswrite:FileName', 'Filename must not contain *.');
        end
        [Directory,file,ext]=fileparts(file);
        if isempty(ext) % add default Excel extension;
            ext = '.xls';
        end
        file = abspath(fullfile(Directory,[file ext]));
        [a1 a2] = fileattrib(file);
        if a1 && ~(a2.UserWrite == 1)
            error('MATLAB:xlswrite:FileReadOnly', 'File cannot be read-only.');
        end
	else
        error('MATLAB:xlswrite:EmptyFileName','Filename is empty.');
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
% Attempt to start Excel as ActiveX server.
try
	xlshandle.Excel = actxserver('Excel.Application');
catch exception
    error('MATLAB:xlswrite:NoCOMServer', 'Could not start Excel server for export.');
end
%------------------------------------------------------------------------------
try
    bCreated = ~exist(file,'file');
    ExecuteWrite;
catch exception
    if (bCreated && exist(file, 'file') == 2)
        delete(file);
    end
    success = false;
    message = exceptionHandler(nargout, exception);
end        
    function ExecuteWrite
            if bCreated
                % Create new workbook.  
                %This is in place because in the presence of a Google Desktop
                %Search installation, calling Add, and then SaveAs after adding data,
                %to create a new Excel file, will leave an Excel process hanging.  
                %This workaround prevents it from happening, by creating a blank file,
                %and saving it.  It can then be opened with Open.
                ExcelWorkbook = xlshandle.Excel.workbooks.Add;
                switch ext
                    case '.xls' %xlExcel8 or xlWorkbookNormal
                       xlFormat = -4143;
                    case '.xlsb' %xlExcel12
                       xlFormat = 50;
                    case '.xlsx' %xlOpenXMLWorkbook
                       xlFormat = 51;
                    case '.xlsm' %xlOpenXMLWorkbookMacroEnabled 
                       xlFormat = 52;
                    otherwise
                       xlFormat = -4143;
                end
                ExcelWorkbook.SaveAs(file, xlFormat);
                ExcelWorkbook.Close(false);
            end

            %Open file
            xlshandle.ExcelWorkbook = xlshandle.Excel.workbooks.Open(file);
            if xlshandle.ExcelWorkbook.ReadOnly ~= 0
                %This means the file is probably open in another process.
                error('MATLAB:xlswrite:LockedFile', 'The file %s is not writable.  It may be locked by another process.', file);
            end
    end
end
%------------------------------------------------------------------------------
function [absolutepath]=abspath(partialpath)

% parse partial path into path parts
[pathname filename ext] = fileparts(partialpath);
% no path qualification is present in partial path; assume parent is pwd, except
% when path string starts with '~' or is identical to '~'.
if isempty(pathname) && isempty(strmatch('~',partialpath))
    Directory = pwd;
elseif isempty(regexp(partialpath,'(.:|\\\\)','once')) && ...
        isempty(strmatch('/',partialpath)) && ...
        isempty(strmatch('~',partialpath));
    % path did not start with any of drive name, UNC path or '~'.
    Directory = [pwd,filesep,pathname];
else
    % path content present in partial path; assume relative to current directory,
    % or absolute.
    Directory = pathname;
end

% construct absulute filename
absolutepath = fullfile(Directory,[filename,ext]);
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


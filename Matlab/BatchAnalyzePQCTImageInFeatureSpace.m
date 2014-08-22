function BatchAnalyzePQCTImageInFeatureSpace(imageList)

% Read image list.
fid = fopen(imageList);
if( fid == -1 )
    error(['ERROR: Can not open ' imageList '.']);
end

fprintf('Reading....\n');
line = fgetl(fid);
lineCount = 1;

while( isstr(line) )
    if ( line(1:2)~='%%' )  % comment character.
        if(lineCount==1) dataPath = line;
        else
            %value = sscanf(line,'%[^\n]');
            value = strread(line, '%s');
            subjectStruct{lineCount-1}.fileName = value{1};
        end
        lineCount = lineCount + 1;
        fprintf([line, '\n']);
    end
    line = fgetl(fid);
end
fclose(fid);

% Run analysis.
for (i=1:lineCount-2)
    if(~isdir(subjectStruct{i}.fileName)) 
        mkdir(subjectStruct{i}.fileName);
end
    cd(subjectStruct{i}.fileName);
    tissueLabelImage = ...
        AnalyzePQCTImageInFeatureSpace(dataPath, subjectStruct{i}.fileName);
    cd('..');
    close all;
end


end

function image = ReadPQCTImage( filename )
% Read a PQCT image type.

% Open file.
fid = fopen( filename );
% Read header first.
header = fread(fid, 1609, '*uint8');
% Read data.
data = fread(fid, '*int16');
fclose(fid);
% Determine image size.
matrixLength = sqrt( length(data) );
% Form the image.
image = reshape( data, [matrixLength matrixLength]);

end
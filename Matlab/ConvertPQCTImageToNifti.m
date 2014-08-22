function niftiImage = ConvertPQCTImageToNifti( pqctFilename, voxelSize )
% syntax: niftiImage = ConvertPQCTImageToNifti( pqctFilename, voxelSize );

% Read PQCT.
pqctImage = ReadPQCTImage( pqctFilename );

% Make nifti header.
niftiImage = make_nii(pqctImage, voxelSize, [0 0 0], 4, []);

% Save nifti.
save_nii(niftiImage, [ pqctFilename, '.nii']);

end
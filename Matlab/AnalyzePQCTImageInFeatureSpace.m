function tissueLabelImage = AnalyzePQCTImageInFeatureSpace(dataPath, filename)
% syntax: AnalyzePQCTImageInFeatureSpace(filename);

%dataPath = '/home/makrogianniss/Data/Sardinia/raw.092410/';
nBins = 256;
nComponents = 4; %4:38%,66%; 4:4%
backgroundThreshold = -200; % -100:4% -200:38%,66%
boneThreshold = 1400; %400; 1500
denoising = 'median'; % options are: '', 'median', 'wiener'
clusteringMethod = 'emgmmbaycls'; % options are: 'k-means', 'fcm', 'emgmmbaycls'
outputPath = './';

% Read Image.
inputImage = ReadPQCTImage( [dataPath filename] );

inputImage = 1724*(double(inputImage)/1000) - 322;

figure, subplot(211), imagesc(inputImage), colormap gray, colorbar, axis image, axis off, ...
%     title(['Input Image of ', filename]);
    title(['Input Image'], 'fontsize', 14);
    
subplot(212), hist(double(inputImage(:)), nBins), ...
%     title(['Intensity Histogram of ', filename]);
    title(['Intensity Histogram'], 'fontsize', 14);
saveas(gcf, [outputPath, filename, '_', 'InputImage', '.png']);

% Denoise image.
switch(lower(denoising))
    case('median')
        filteredImage = medfilt2(inputImage);
    case('wiener')
        filteredImage = wiener2(inputImage);
    otherwise
        filteredImage = inputImage;
end

switch(lower(denoising))
    case{'median','wiener'}
        figure, subplot(211), imagesc(filteredImage), colormap gray, colorbar, axis image, axis off, ...
%             title(['Denoised Image of', filename]);
            title(['Denoised Image'], 'fontsize', 14);    
            
        subplot(212), hist(double(filteredImage(:)), nBins), ...
%             title(['Intensity Histogram of ', filename]);
            title(['Intensity Histogram'], 'fontsize', 14);     
            
        saveas(gcf, [outputPath, filename, '_', denoising, '_', 'DenoisedImage', '.png']);
    otherwise
end

% filteredImage = inputImage;

% Background threshold.
foregroundMask = filteredImage > backgroundThreshold;
foregroundImage = foregroundMask .* double(filteredImage);

% Identify bone.
boneMask = filteredImage > boneThreshold;

% Find indices and values of fat and muscle voxels.
fatandmuscleMask = foregroundMask - boneMask;
fatandmuscleImage = fatandmuscleMask .* double(filteredImage);
% Form fat and muscle vector.
[fatandmuscleIndices.x,  fatandmuscleIndices.y, fatandmuscleVector] = ...
    find( fatandmuscleImage );
fatandmuscleVector = double(fatandmuscleVector);

figure, subplot(211), axis off, ...
    imagesc(fatandmuscleImage, [-400, boneThreshold]), ...
    colormap gray, colorbar, axis image,  axis off, ...
%     title(['Fat and Muscle Image of ', filename]);
title(['Fat, Muscle and Bone Image'], 'fontsize', 14);

subplot(212), hist(double(fatandmuscleVector(:)), nBins)
axis([backgroundThreshold boneThreshold 0 2000]), ...
%     title(['Intensity Histogram of ', filename]);
    title(['Intensity Histogram'], 'fontsize', 14);
saveas(gcf, [outputPath, filename, '_', denoising, '_', ...
    'FatandMuscleImage', '.png']);

% Analyze by:
switch lower(clusteringMethod)
    case{'k-means'}
        % Apply k-means.
        opts = statset('Display','iter');
        tissueIdx = kmeans( fatandmuscleVector', nComponents, ...
            'Start', 'Uniform', 'Options', opts );
    case{'fcm'}
        data.X = fatandmuscleVector;
        param.c = nComponents;
        result = FCMclust(data,param);
        [~, tissueIdx] = max(result.data.f');
    case('emgmmbaycls') %emgmm method with bayesian decision.
        % EM and GMM.
        STPR_PATH = getenv('STPR_PATH');
        stprpath(STPR_PATH);
        
        initial_model = struct('Mean',[-22 72 514 993],'Cov',[32^2 42^2 72^2 302^2],'Prior',[0.32 0.61 0.04 0.03]);
        model = emgmm(fatandmuscleVector', struct('ncomp', nComponents, ...
            'verb',1, 'init', 'cmeans', 'cov_type', 'full')); % For 66%
%         model = emgmm(fatandmuscleVector', struct('ncomp', nComponents, ...
%             'verb',1, 'init', 'cmeans', 'cov_type', 'full'));
        disp(model);
        
        figure, % ppatterns(fatandmuscleVector')
%         h2=pgmm(initial_model,struct('color','b'));
        h2=pgmm(model,struct('color','b'));
        set(h2,'linewidth', 2)
        axis([backgroundThreshold boneThreshold 0 0.01]),  ...
%         xlabel('Density $mg/cm^3$','Interpreter','latex', 'fontsize', 14);
        xlabel('Density mg/cm^3', 'fontsize', 14);
        ylabel('Probability', 'fontsize', 14);
%         title(['Fat and Muscle Intensity Histogram of ', filename])
        title('66% Tibia Site - Tissue Density Model', 'fontsize', 16)
        
        saveas(gcf, [outputPath, filename, '_', denoising, '_', ...
            clusteringMethod, '_', 'HistogramModel', '.png']);
        saveas(gcf, [outputPath, filename, '_', denoising, '_', ...
            clusteringMethod, '_', 'HistogramModel', '.fig']);
        
        for i=1:nComponents
            bayesModel.Pclass{i}.Mean = model.Mean(:,i);
            bayesModel.Pclass{i}.Cov = model.Cov(:,:,i);
            bayesModel.Pclass{i}.fun = model.fun;
            bayesModel.Pclass{i}.Prior = model.Prior(i);
            bayesModel.Prior(i) = model.Prior(i);
        end
        
        tissueIdx = bayescls(fatandmuscleVector', bayesModel);
        tissueIdx = tissueIdx';
end

% Map voxels back to the image space.
tissueLabelImage = zeros(size(inputImage));
for i=1:length(fatandmuscleVector)
    tissueLabelImage(fatandmuscleIndices.x(i),  fatandmuscleIndices.y(i)) = ...
        tissueIdx(i);
end

figure, imagesc(tissueLabelImage), colorbar, axis image,  ...
    title(['Separated Fat and Muscle Image of ', filename]);
saveas(gcf, [outputPath, filename, '_', denoising, '_', ...
    clusteringMethod, '_', 'SegmentedFatMuscleImage', '.png']);

end
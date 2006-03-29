% mksubfov.m
%
% $Id: mksubfov.m,v 1.1 2006/03/29 02:16:59 greve Exp $
%
% The purpose of this matlab script is to create the
% mni305.cor.subfov1.mgz and mni305.cor.subfov2.mgz volumes.  These
% are the same data as found in mni305.cor.mgz, but the field-of-view
% is much smaller and only covers the brain. The purpose of this is to
% reduce the amount of space needed to store the data. This is
% especially important when doing group functional analysis where
% there might be many subjects combined. In one subfov (subfov1), the
% voxel size is 1mm isotropic, 147 x 147 x 184. In the other (yep,
% subfov2), its 2mm isotropic,  74 x 74 x 92. These volumes are in
% register with mni305.cor.mgz in that the share the same RAS
% space, ie, you can run:
%   tkregister2 --targ mni305.cor.mgz --mov mni305.cor.subfov1.mgz \
%        --regheader --reg /tmp/reg
% And the volumes will be in register.
%
% You should be in $DEV/distribution/averages when running this script

% Load in the pixel values. These are 1mm, isotropic
cor = MRIread('mni305.cor.mgz');
if(isempty(cor)) 
  fprintf('ERROR: cannot find mni305.cor.mgz\n');
  fprintf('Make sure you are in $DEV/distribution/averages\n');
  return; 
end

% Binary mask of the brain.
m = MRIread('mni305.mask.cor.mgz');
if(isempty(m)) 
  fprintf('ERROR: cannot find mni305.mask.cor.mgz\n');
  fprintf('Make sure you are in $DEV/distribution/averages\n');
  return; 
end

% Create a bounding box from the mask, expanded by 2 voxels
ind = find(m.vol);
[r c s] = ind2sub(m.volsize,ind); % These are all 1-based
rmin = min(r)-2;
rmax = max(r)+2;
fovr = rmax - rmin;
cmin = min(c)-2;
cmax = max(c)+2;
fovc = cmax - cmin;
smin = min(s)-2;
smax = max(s)+2;
fovs = smax - smin;
fovrc = max(fovr,fovc);

% 1mm isotropic SubFOV ---------------------------------------
crsP0 = [cmin rmin smin 1]';  % crs at first voxel of FOV
P0 = m.vox2ras1*crsP0; % use vox2ras1 since crs are 1-based
D = diag(m.volres); % Diagonal matrix of voxel sizes
vox2ras = [m.Mdc*D P0(1:3); 0 0 0 1]; % but this is 0-based
cor2 = cor;
cor2.vol = cor.vol(rmin:rmin+fovrc,cmin:cmin+fovrc,smin:smax);
cor2.vox2ras0 = vox2ras;
MRIwrite(cor2,'mni305.cor.subfov1.mgz');

% 2mm isotropic SubFOV ---------------------------------------
crsP0 = [cmin rmin smin 1]';
P0 = m.vox2ras1*crsP0; % use vox2ras1 since crs are 1-based
D = 2*diag(m.volres); % use 2-times volres
vox2ras = [m.Mdc*D P0(1:3); 0 0 0 1];
cor2 = cor;
cor2.vol = cor.vol(rmin:2:rmin+fovrc,cmin:2:cmin+fovrc,smin:2:smax);%skip2
cor2.vox2ras0 = vox2ras;
MRIwrite(cor2,'mni305.cor.subfov2.mgz');


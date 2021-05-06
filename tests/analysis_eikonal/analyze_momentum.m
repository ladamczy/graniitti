% Read eikonal amplitudes in momentum space
%
% mikael.mieskolainen@cern.ch, 2019
close all; clear; 

addpath ../mcodes

fig0 = figure;

%while (true)

% Run this first
setup;

% Differential cross section dsigma/dt
Xt = {};
Xb = {};
legends = {};
for i = 1:length(sqrts)
    
    % Read amplitude array file
    [~,filename] = system(sprintf('ls ../../eikonal/MSA_2212_2212_%0.0f_*', sqrts(i)));
    filename = filename(1:end-1); % Remove \n
    Xt{i} = csvread(filename);
    
    if (sqrts(i) < 1000)
        legends{i} = sprintf('$\\sqrt{s} = %0.0f$ GeV', sqrts(i));
    elseif (sqrts(i) < 1900)
        legends{i} = sprintf('$\\sqrt{s} = %0.1f$ TeV', sqrts(i)/1e3);
    elseif (sqrts(i) < 7000)
        legends{i} = sprintf('$\\sqrt{s} = %0.2f$ TeV', sqrts(i)/1e3);
    else
        legends{i} = sprintf('$\\sqrt{s} = %1.0f$ TeV', sqrts(i)/1e3);    
    end
end

% Generate dsigma/dt
dxs = {};
for i = 1:length(sqrts)
    dxs{i} = abs( Xt{i}(:,Re_ind) + 1i*Xt{i}(:,Im_ind) ).^2 / (16*pi*sqrts(i)^4) * GeV2mb;
end

% Plot visualization scale indicators
tval = 3.5; % |t| position
for i = 1:length(sqrts)
    
    % Find the index for suitable x coordinate
    [~, ind] = min(abs(Xt{i}(:,x_ind) - tval));
    
    if (factor(i) >= 1) 
        txt = sprintf('$\\times \\, %0.0f$', factor(i));
    elseif (factor(i) > 0.001)
        txt = sprintf('$\\times \\, %0.2f$', factor(i));    
    else
        txt = sprintf('$\\times \\, %0.3f$', factor(i));
    end
    yscale = 1.6; % Visualization scale
    coord = factor(i)*dxs{i}(ind)*yscale;
    
    text(tval, coord, txt, 'interpreter','latex'); hold on;
end

% ------------------------------------------------------------------------
%% Plot simulations
for i = 1:length(sqrts)
    
    % Calculate probability normalized t-spectrum
    P = dxs{i}; P = P / sum(P);
    
    % Total xs via optical theorem: sigma_tot(s) = 1/s Im A(s,t=0)
    sigma_tot = Xt{i}(1,Im_ind) / (sqrts(i)^2) * GeV2mb;
    
    % Elastic xs via numerical integral
    sigma_el  = trapz(Xt{i}(:,1), dxs{i});
    
    % Shannon entropy
    S1 = -sum(P .* log(P));
    
    % Alternative entropy formulation via numerical integral
    xval =  Xt{i}(:,1);
    yval = -1/(sigma_el) * dxs{i} .* log(1/(sigma_tot*sigma_el) * dxs{i});
    S2 = trapz(xval, yval);
    
    fprintf('%0.1f, tot: %0.1f mb, el: %0.1f mb, Discrete Shannon entropy: %0.3f (nats), S2: %0.3f \n', ...
        sqrts(i), ...
        sigma_tot, ...
        sigma_el, ...
        S1, ...
        S2);
    
    % Plot
    plot(Xt{i}(:,1), factor(i)*dxs{i}, '-', 'linewidth', 1.1); hold on;
end
set(gca,'yscale','log');
%set(gca,'xscale','log');
%axis tight;
%%
% ------------------------------------------------------------------------
% Read in ISR data
%{
[~, index] = min(abs(sqrts - 62));

if (sqrts(1) < 63)
ISR = dlmread('../../HEPData/EL/ISR_62.csv');

errorbar(ISR(:,1), ISR(:,4)*factor(index), ...
         sqrt(ISR(:,6).^2 ), ...
         sqrt(ISR(:,5).^2 ), '.', 'color', [1 1 0]*0.3, 'CapSize', 1);
legends{length(legends)+1} = 'ISR $\sqrt{s} = 62.5$ GeV';
index = index + 1;
end
%}

% ------------------------------------------------------------------------
% Read in Fermilab data
% 
[~, index] = min(abs(sqrts - 546));

FNAL = dlmread('../../HEPData/EL/FNAL_546.csv');

errorbar(FNAL(:,1), FNAL(:,4)*factor(index), ...
         sqrt(FNAL(:,6).^2 ), ...
         sqrt(FNAL(:,5).^2 ), '.', 'color', [0 1 0]*0.5, 'CapSize', 1);
legends{length(legends)+1} = 'CDF (E741) $\sqrt{s} = 546$ GeV';

% Read in SPS data
% 
SPS  = dlmread('../../HEPData/EL/SPS_546.csv');

errorbar(SPS(:,1), SPS(:,4)*factor(index), ...
         sqrt(SPS(:,6).^2 ), ...
         sqrt(SPS(:,5).^2 ), '.', 'color', [0 0 1]*0.5, 'CapSize', 1);
legends{length(legends)+1} = 'UA4 (SPS) $\sqrt{s} = 546$ GeV';

% ------------------------------------------------------------------------
% Read in Fermilab data
% Measurement of small angle antiproton-proton elastic scattering at√ s= 546 and 1800 GeV
[~, index] = min(abs(sqrts - 1800));

ABE = dlmread('../../HEPData/EL/ABE_1994_1800.csv');

errorbar(ABE(:,1), ABE(:,4)*factor(index), ...
         sqrt(ABE(:,6).^2 ), ...
         sqrt(ABE(:,5).^2 ), '.', 'color', [1 0 0]*0.5, 'CapSize', 1);
legends{length(legends)+1} = 'CDF (E741) $\sqrt{s} = 1.8$ TeV';
index = index + 1;


% ------------------------------------------------------------------------
% Read in TOTEM data
% Proton-proton elastic scattering at the {LHC} energy of $\sqrt{s} = 7$ {TeV}
[~, index] = min(abs(sqrts - 7000));

TOTEM_high_t = dlmread('../../HEPData/EL/TOTEM_7.csv');
TOTEM_low_t  = dlmread('../../HEPData/EL/TOTEM_7_low_t.csv');

TOTEM = [TOTEM_low_t; TOTEM_high_t];

errorbar(TOTEM(:,1), TOTEM(:,4) , ...
         sqrt(TOTEM(:,6).^2 + TOTEM(:,8).^2), ...
         sqrt(TOTEM(:,5).^2 + TOTEM(:,7).^2), 'k.', 'CapSize', 1);
legends{length(legends)+1} = 'TOTEM $\sqrt{s} = 7$ TeV';
index = index + 1;

% ------------------------------------------------------------------------
% 3-gluon exchange

tval = linspace(1.5,8,1000);
scale = 2e-4; % Arbitrary visualization scale
plot(tval, scale*tval.^(-8), 'k-.');

legends{length(legends)+1} = '$t^{-8}$';
% ------------------------------------------------------------------------


l = legend(legends); set(l,'interpreter','latex','fontsize', 8); legend('boxoff');
set(l,'interpreter','latex'); legend('boxoff');
xlabel('$-t$ (GeV$^2$)','interpreter','latex');
ylabel('$d\sigma/dt$ (mb/GeV$^2$)','interpreter','latex');
axis square;
axis([0 3.5 1e-10 1e6]);
xticks(linspace(0, 8, 17));

% FOR OPTIMIZATION LOOP
%drawnow;
%pause(3);
%clf();
%end

%h = versioninfo('../../VERSION.json');

% PRINT OUT
filename = sprintf('./figs/t_space_dsigmadt.pdf');
print(fig0, '-dpdf', filename);
system(sprintf('pdfcrop --margins ''10 10 10 10'' %s %s', filename, filename));


%% Local B-slope (two figures)

fig{1} = figure;
fig{2} = figure;

partial = {};

legends = {};
for i = 1:length(sqrts)
    legends{i} = sprintf('$\\sqrt{s} = %0.0f$ GeV', sqrts(i) );
end

for k = 1:2
    figure(fig{k});
    % Restart color order
    ax = gca;
    ax.ColorOrderIndex = 1;
    
    %plot(linspace(0,4,2), zeros(2,1), 'k-'); hold on;
    
    for i = 1:length(dxs)
        % d/dt ln(dsigma/dt)
        delta = Xt{i}(2,1) - Xt{i}(1,1);
        partial{i} = - diff(log(dxs{i})) / delta;
        
        plot(Xt{i}(1:end-1,1), partial{i}); hold on;
    end
    if (k == 1)
        
        % Horizontal line
        plot(linspace(1e-3,10,10), zeros(10,1), 'k-'); hold on;
        
        axis([0 4 -20 40]);
        set(gca,'xtick', linspace(0,4,11));
        l = legend(legends); set(l,'interpreter','latex'); legend('boxoff'); 
    end
    if (k == 2)
        axis([0 0.25 5 40]);
        %set(gca,'ytick', linspace(10,35,11));
        l = legend(legends); set(l,'interpreter','latex'); legend('boxoff');
    end
    
    xlabel('$-t$ (GeV$^2$)','interpreter','latex'); axis square;
    ylabel('$B(t) \equiv \frac{d}{dt}\ln(d\sigma/dt)$ (GeV$^{-2}$)','interpreter','latex');

    % PRINT OUT
    filename = sprintf('./figs/t_space_bslope_%d.pdf', k);
    print(fig{k}, '-dpdf', filename);
    system(sprintf('pdfcrop --margins ''10 10 10 10'' %s %s', filename, filename));
end

%axis([1e-2 4 -10 10]); axis square;


%% B(t = fixed)

fig0 = figure;

tvals = [0 0.02 0.05];
legs = {};

for k = 1:length(tvals)

    B_t0 = zeros(length(sqrts),1);
    % Find the index for suitable x coordinate
    [~, bin] = min(abs(Xt{i}(:,x_ind) - tvals(k)));
    
    for i = 1:length(sqrts)
       B_t0(i) = partial{i}(bin);
    end
    plot(sqrts, B_t0, 's-'); hold on;
    legs{end+1} = sprintf('$|t| = %0.2f$ GeV$^2$', Xt{1}(bin, x_ind));
end
l = legend(legs);
set(l,'interpreter','latex','location','southeast');
legend('boxoff');

set(gca,'xscale','log');

xlabel('$\sqrt{s}$ (GeV)','interpreter','latex');
ylabel('$B(t)$ (GeV$^{-2}$)','interpreter','latex');
axis square; axis tight;

% PRINT OUT
filename = sprintf('./figs/t_space_bt0.pdf');
print(fig0, '-dpdf', filename);
system(sprintf('pdfcrop --margins ''10 10 10 10'' %s %s', filename, filename));


%% Elastic amplitudes in t-space

fig0 = figure;

% Undersampling
step = 2;

for i = 1:length(sqrts)
    plot(Xt{i}(1:step:end,1), Xt{i}(1:step:end,Im_ind) / s(i), '-'); hold on;
end
% Restart color order
ax = gca;
ax.ColorOrderIndex = 1;
for i = 1:length(sqrts)
    plot(Xt{i}(1:step:end,1), Xt{i}(1:step:end,Re_ind) / s(i), '--'); hold on;
end

title('Re[A] (dashed), Im[A] (solid)','interpreter','latex');

%set(gca,'yscale','log');
set(gca,'xscale','log');
axis tight; axis square;
axis([1e-2 4 -20 inf]);

% Horizontal line
plot(linspace(1e-3,10,10), zeros(10,1), 'k-'); hold on;

l = legend(legends); set(l,'interpreter','latex'); legend('boxoff');
xlabel('$-t$ (GeV$^2$)','interpreter','latex');
ylabel('$A_{el}(s,t) / s$','interpreter','latex');

% PRINT OUT
filename = sprintf('./figs/t_space_amplitude.pdf');
print(fig0, '-dpdf', filename);
system(sprintf('pdfcrop --margins ''10 10 10 10'' %s %s', filename, filename));


%% Real and Imag parts of elastic amplitudes in t-space (SPIRAL)

fig0 = figure;
for i = 1:length(sqrts)
    A = (Xt{i}(:,Re_ind) + 1i*Xt{i}(:,Im_ind))/ s(i);
    plot(real(A), imag(A), '-'); hold on; axis square; axis tight;
    if i == length(sqrts)
        for k = 1:45:length(GeV2fm*Xt{i}(:,1))/8
            text(real(A(k)), imag(A(k)), sprintf('%0.2f', -Xt{i}(k,1)), 'interpreter','latex');
        end
    end
end
xlabel('Re [$A_{el}(s,t) / s$]','interpreter','latex');
ylabel('Im [$A_{el}(s,t) / s$]','interpreter','latex');
set(gca,'xtick',linspace(-5,5,11));
axis([-5 5 -10 100]);


%% Re{A_el(s,t)}/Im{A_el(s,t)}

fig0 = figure;

for i = 1:length(sqrts)
    A = (Xt{i}(:,Re_ind) + 1i*Xt{i}(:,Im_ind))/ s(i);
    plot(Xt{i}(:,1), real(A) ./ imag(A), '-'); hold on;
end
axis square; axis tight;
%set(gca,'yscale','log');

% Horizontal line
plot(linspace(1e-3,10,10), zeros(10,1), 'k-'); hold on;

xlabel('$-t$ (GeV$^2$)','interpreter','latex');
ylabel('Re [$A_{el}(s,t)]$ / Im [$A_{el}(s,t)]$','interpreter','latex');
%set(gca,'xscale','log');
l = legend(legends); set(l,'interpreter','latex'); legend('boxoff');

axis([1e-2 4 -15 15]);


%% Re{A_el(s,t)}/Im{A_el(s,t)} (ZOOMED)

fig0 = figure;

for i = 1:length(sqrts)
    A = (Xt{i}(:,Re_ind) + 1i*Xt{i}(:,Im_ind))/ s(i);
    plot(Xt{i}(:,1), real(A) ./ imag(A), '-'); hold on;
end
axis square; axis tight;
%set(gca,'yscale','log');

% ZOOM IN
axis([0 0.1 0 0.14]);

xlabel('$-t$ (GeV$^2$)','interpreter','latex');
ylabel('Re [$A_{el}(s,t)]$ / Im [$A_{el}(s,t)]$','interpreter','latex');
%set(gca,'xscale','log');
l = legend(legends); set(l,'interpreter','latex'); legend('boxoff');

% PRINT OUT
filename = sprintf('./figs/t_space_amplitude_re_im_ratio_zoom.pdf');
print(fig0, '-dpdf', filename);
system(sprintf('pdfcrop --margins ''10 10 10 10'' %s %s', filename, filename));



%% Rho-parameter rho = Re{A_el(s,t=0)}/Im{A_el(s,t=0)}

fig0 = figure;

% Construct rho
rho = zeros(length(Xt),1);
tvals = [0.0 0.02 0.05 0.10];
legs = {};
for k = 1:length(tvals)
    
    % Find the index for suitable x coordinate
    [~, bin] = min(abs(Xt{i}(:,x_ind) - tvals(k)));
    
    for i = 1:length(sqrts)
        rho(i) = Xt{i}(bin, Re_ind) / Xt{i}(bin,Im_ind);
    end
    plot(sqrts, rho, 's-'); hold on;
    legs{end+1} = sprintf('$|t| = %0.2f$ GeV$^2$', Xt{1}(bin, x_ind));
end
l = legend(legs); legend('boxoff');
set(l,'interpreter','latex');

%set(gca,'yscale','log');
set(gca,'xscale','log');
axis tight; axis square;

xlabel('$\sqrt{s}$ (GeV)','interpreter','latex');
ylabel('$\rho(t) \equiv$ Re [$A_{el}(s,t)]/$ Im [$A_{el}(s,t)$]','interpreter','latex');
axis([0 inf 0 0.14]);

% PRINT OUT
filename = sprintf('./figs/t_space_rho.pdf');
print(fig0, '-dpdf', filename);
system(sprintf('pdfcrop --margins ''10 10 10 10'' %s %s', filename, filename));


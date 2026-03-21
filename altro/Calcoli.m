
%% Vectors initialization

dati = readtable('datif.csv');

vect_speed = dati{: , 4};
vect_volt = dati{: , 2};
vect_time = (dati{: , 1}) / 1e6;
vect_current = dati{: , 3};
vect_dt = diff(vect_time);

speed_area = ((vect_speed(1:end-1) + vect_speed(2:end)) ./2 ) .* vect_dt;
x = [0 ; cumsum(speed_area)];

vect_power = vect_volt .* vect_current;
power_area = ((vect_power(1:end-1) + vect_power(2:end))./2) .*vect_dt;

energy = [0 ; cumsum(power_area)];

%% Plotting initial vectors

figure('Name', 'Analisi Dati Telemetria', 'Color', 'w');

% Grafico 1: Velocità su Distanza
subplot(5, 1, 1);
plot(x, vect_speed, 'LineWidth', 1.5);
title('Velocità vs Distanza');
ylabel('Velocità [m/s]');
grid on;

% Grafico 2: Potenza su Distanza
subplot(5, 1, 2);
plot(x, vect_power, 'r', 'LineWidth', 1);
title('Potenza vs Distanza');
ylabel('Potenza [W]');
grid on;

% Grafico 3: Energia su Distanza
subplot(5, 1, 3);
plot(x, energy, 'g', 'LineWidth', 1.5);
title('Energia Consumata vs Distanza');
ylabel('Energia [J]');
grid on;

% Grafico 4: Tensione su Distanza
subplot(5, 1, 4);
plot(x, vect_volt, 'm', 'LineWidth', 1);
title('Tensione vs Distanza');
ylabel('Tensione [V]');
grid on;

% Grafico 5: Corrente su Distanza
subplot(5, 1, 5);
plot(x, vect_current, 'k', 'LineWidth', 1);
title('Corrente vs Distanza');
xlabel('Distanza [m]'); % L'etichetta dell'asse X si mette sull'ultimo grafico
ylabel('Corrente [A]');
grid on;

%% CALCOLO GIRI MIGLIORI:

TIME_LIMIT = 190.9;
NUM_LAPS = 3;
energy_best1 = inf;
energy_best2 = inf;



for i = 1 : NUM_LAPS
    
    if current_time < TEMPO_LIMITE
        
        % Caso 1: Primo giro valido (energia_best1 è inf/None)
        if isinf(energy_best1)
            energy_best1 = energia_last;
            energy_best2 = energia_last;
            
            % Copio dati nel Giro 1
            speed_lap1 = velocita_arr;
            voltage_lap1 = voltage_arr;
            current_lap1 = current_arr;
            time_lap1 = tempo_arr;
            x_lap1 = x_arr;
            power_lap1 = potenza_arr;
            
            % Copio dati nel Giro 2
            speed_lap2 = speed_arr;
            voltage_lap2 = voltage_arr;
            current_lap2 = current_arr;
            time_lap2 = time_arr;
            x_lap2 = x_arr;
            power_lap2 = power_arr;
            
        % Caso 2: Nuovo record assoluto (migliore del Best 1)
        elseif energy_last < energy_best1
            % Sposto il vecchio Best 1 nel Best 2
            energy_best2 = energy_best1;
            
            speed_lap2 = speed_lap1;
            voltage_lap2 = voltage_lap1;
            current_lap2 = current_lap1;
            time_lap2 = time_lap1;
            x_lap2 = x_lap1;
            power_lap2 = power_lap1;
            
            % Salvo il nuovo come Best 1
            energy_best1 = energy_last;
            
            speed_lap1 = speed_arr;
            voltage_lap1 = voltage_arr;
            current_lap1 = current_arr;
            time_lap1 = time_arr;
            x_lap1 = x_arr;
            power_lap1 = power_arr;
            
        % Caso 3: Migliore del Best 2 ma peggiore del Best 1
        elseif (energy_last < energy_best2 && energy_last > energy_best1)
            % Aggiorno solo il Best 2
            energy_best2 = energia_last;
            
            speed_lap2 = speed_arr;
            voltage_lap2 = voltage_arr;
            current_lap2 = current_arr;
            time_lap2 = time_arr;
            x_lap2 = x_arr;
            power_lap2 = power_arr;
        end
    end
end

% Output di controllo
fprintf('Miglior Energia 1: %.2f\n', energy_best1);
fprintf('Miglior Energia 2: %.2f\n', energy_best2);




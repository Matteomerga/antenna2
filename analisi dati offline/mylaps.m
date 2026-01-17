function [M] = mylaps(Data)
    M = cell(1,11);
    l = 1;
    prev = 0;
    startl = 0;
    endl = 0;
    th = 0.05;   % soglia (float)
    
    for t = 1:length(Data(:,1))
        speed = Data(t,4);

        if prev == 0 && speed > 0
            startl = t;    
        end

        if prev > 0 && speed == 0
            endl = t;
        end

        if not(startl == 0) && not(endl == 0)
            lapData = Data(startl:endl, :);

            %energia
            V = lapData(:,2);
            I = lapData(:,3); 
            tLap = lapData(:,1);

            P = V .* I; 
            E = cumtrapz(tLap, P);
            lapData = [lapData E P];

            M{l} = lapData;
            l = l + 1;
            startl = 0; 
            endl = 0;
            %fprintf('lap %d completato\n',l);
        end  

        prev = speed;
    end
end

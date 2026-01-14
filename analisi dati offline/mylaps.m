function[M] = mylaps(Data)
    M = cell(1,11);
    l = 1; % M index
    prev = 0;
    startl=0;
    endl=0;
    for t = 1:length( Data(:,1) )
        curr = Data(t,4);
    
        if prev==0 && curr > 0
            startl = t;    
        end
    
        if prev > 0 && curr == 0
            endl = t;
        end
        
        if not(startl == 0) && not(endl == 0)
            M{l} = Data([startl: endl], :);
            l = l+1;
            startl = 0; 
            endl = 0;
            %fprintf('lap %d completato\n',l);
        end  
        
        prev = curr;
    end
end
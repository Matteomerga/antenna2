
function [L1, L2, L3, L4, L5, L6 ,L7, L8, L9, L10, L11] = laps(Data)
   record = 1:length(Data(:,1));
   index1 = 1;
   index2 = 1;
   Laps = 1:11; 
   recordPos = 1;
   for lap = Laps
        for i = record
            i = recordPos;
            if i >= length(record)
                break
            end
            while Data(i, 4) == 0
                if Data(i+1, 4) > 0
                index1 = i;
                end
                i = i+1;
                if i >= length(record)
                     break
                end
            end
            while Data(i,4) > 0
                if Data(i+1, 4) == 0
                index2 = i+1;
                end
                i = i+1;
                if i >= length(record)
                     break
                end
            end
            recordPos = i;
            break
        end
        if lap == 1
            L1 = Data(index1:index2, :);
        end
        if lap == 2
            L2 = Data(index1:index2, :);
        end
        if lap == 3
            L3 = Data(index1:index2, :);
        end
        if lap == 4
            L4 = Data(index1:index2, :);
        end
        if lap == 5
            L5 = Data(index1:index2, :);
        end
        if lap == 6
            L6 = Data(index1:index2, :);
        end
        if lap == 7
            L7 = Data(index1:index2, :);
        end
        if lap == 8
            L8 = Data(index1:index2, :);
        end
        if lap == 9
            L9 = Data(index1:index2, :);
        end
        if lap == 10
            L10 = Data(index1:index2, :);
        end
        if lap == 11
            L11 = Data(index1:index2, :);
        end
   end
end




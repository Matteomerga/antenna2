Data = readmatrix('datif1.csv');
M = mylaps(Data);

%M = equalize(M);

Lap = cell2mat(M(1));

LapData = uifigure(Name="Lap Data", NumberTitle="off");
g = uigridlayout(LapData); %Cuts the window into a grid
g.RowHeight = {'0.5x', 22, 22, 22, 22, 22, 22, '3x'};
g.ColumnWidth = {100,'1x'};
ax = uiaxes(g); %We need this as the parents for the plot function
ax.Layout.Row = [1 8];
ax.Layout.Column = 2;
plot(ax, Lap(:,1), Lap(:,2), 'LineWidth',2.0);
xl = xlabel(ax, "Time (s)");
yl = ylabel(ax, "Voltage (V)");
LapNumber = uidropdown(g, ...
    Items=["Lap1", "Lap2", "Lap3", "Lap4", "Lap5", "Lap6", "Lap7", "Lap8", "Lap9", "Lap10", "Lap11"], ...
    ItemsData=[1 2 3 4 5 6 7 8 9 10 11]);
Lap2Number = uidropdown(g, ...
    Items=["None", "Lap1", "Lap2", "Lap3", "Lap4", "Lap5", "Lap6", "Lap7", "Lap8", "Lap9", "Lap10", "Lap11"], ...
    ItemsData=[0 1 2 3 4 5 6 7 8 9 10 11]);
lbl = uilabel(g, "Text", "Compare to:");
cbx = uicheckbox(g, "Text", "All");
DataType = uidropdown(g, ...
    Items=["Voltage", "Current", "Speed", "Power", "Position"], ...
    ItemsData=[2 3 4 7 5]);
Compare = uidropdown(g, ...
    Items=["None", "Voltage", "Current", "Speed", "Power"], ...
    ItemsData=[0 2 3 4 7], ...
    ValueChangedFcn=@(src,event) updatePlot(ax, M, LapNumber, DataType, src, Lap2Number, cbx.Value));
DataType.ValueChangedFcn=@(src,event) updatePlot(ax, M, LapNumber, src, Compare, Lap2Number, cbx.Value);
LapNumber.ValueChangedFcn=@(src,event) updatePlot(ax, M, src, DataType, Compare, Lap2Number, cbx.Value);
Lap2Number.ValueChangedFcn=@(src,event) updatePlot(ax, M, LapNumber, DataType, Compare, src, cbx.Value);
cbx.ValueChangedFcn=@(src,event) updatePlot(ax, M, LapNumber, DataType, Compare, Lap2Number, src.Value);

%Poistioning of drop downs
LapNumber.Layout.Row = 2;
LapNumber.Layout.Column = 1;
DataType.Layout.Row = 3;
DataType.Layout.Column = 1;
Compare.Layout.Row = 4;
Compare.Layout.Column = 1;
lbl.Layout.Row = 5;
lbl.Layout.Column = 1;
Lap2Number.Layout.Row = 6;
Lap2Number.Layout.Column = 1;
cbx.Layout.Row = 7;
cbx.Layout.Column = 1;

function updatePlot(ax, M, src, src2, src3, src4, flag)
    lgd = legend(ax);
    xl = xlabel(ax, "Time (s)");
    yl = ylabel(ax, "");
    xl.Visible = "on";
    yl.Visible = "on";
    lgd.Visible = "off";
    src3.Enable = "on";
    val = src.Value; %Get value of first dropdown: The laps
    D = src2.Value; %Get value of second dropdown: The data type
    C = src3.Value; %Get value of third dropdown: Compare with what datatype
    C2 = src4.Value; %Get value of fourth dropdown: Compare with another Lap
    if flag == 1
        src3.Value = 0;
        src3.Enable = "off";
        plotEverything(ax, M, D);
        return
    end
    data = cell2mat(M(val)); %NOT IN ANY IF STATEMENTS
    if D == 5
        plot(ax, data(:,5),data(:,6), 'LineWidth',2.0);
        xl.Visible = "off";
        return
    end
    if C2~=0
        src3.Value = 0;
        src3.Enable = "off";
        data2 = cell2mat(M(C2));
        plot(ax, data(:,1), data(:,D), data2(:,1), data2(:,D), 'LineWidth',2.0);
        xl.String = "Time (s)";
        switch D
        case 2
            yl.String = "Voltage (V)";
        case 3
            yl.String = "Current (mA)";
        case 4
            yl.String = "Speed (Km/h)";
        end
        return
    end
    if C~=0
        plot(ax, data(:,1),data(:,D),data(:,1),data(:,C), 'LineWidth',2.0);
        yl.Visible = "off";
        switch D
        case 2
            legend1 = "Voltage (V)";
        case 3
            legend1 = "Current (mA)";
        case 4
            legend1 = "Speed (Km/h)";
        end
        switch C
        case 2
            legend2 = "Voltage (V)";
        case 3
            legend2 = "Current (mA)";
        case 4
            legend2 = "Speed (Km/h)";
        end
        lgd.Visible = "on";
        lgd.String = [legend1, legend2];
        return
    end
    plot(ax, data(:,1),data(:,D), 'LineWidth',2.0);
    xl.String = "Time (s)";
    switch D
    case 2
        yl.String = "Voltage (V)";
    case 3
        yl.String = "Current (mA)";
    case 4
        yl.String = "Speed (Km/h)";
    end
end

function plotEverything(ax, M, D)
    
    for e=1:11
        if isempty(cell2mat(M(e)))
            break
        end
        Lap = cell2mat(M(e));
        plot(ax, Lap(:,1), Lap(:,D));
        ax.NextPlot = "add";
    end
    ax.NextPlot = "replacechildren";
end

function Me = equalize(M)
    Me = cell(1,11);
    for n = 1:11
        s = M(n, 1, 1);
        Me(n) = (M(n, :, 1)-s)/1e6;
    end
end

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
        
        if not(startl == 0) && not(endl == 0) %I modified the code here a bit to normalize the x axis
            temp = Data(startl: endl, :);
            smallTime = temp(1,1);
            temp(:,1) = (temp(:,1)-smallTime)/1e6;
            temp(:,7) = temp(:,2).*temp(:,3);
            M{l} = temp;
            l = l+1;
            startl = 0; 
            endl = 0;
            %fprintf('lap %d completato\n',l);
        end  
        
        prev = curr;
    end
end
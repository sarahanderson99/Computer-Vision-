oldpoint = [0 0 0 1]';

newpoints = zeros(13,4);

i = 1;
while( i<= length(cameraParams.TranslationVectors) )
    trans = cameraParams.TranslationVectors(i,:);
    rota = cameraParams.RotationVectors(i,:);

    display(trans);
    display(rota);

    translation = ([1 0  0 -trans(1); 0 1 0 -trans(2); 0 0 1 trans(3); 0 0 0 1]);
    rotation = [rotationVectorToMatrix([-rota(1) -rota(2) rota(3)]) zeros(3,1); 0 0 0 1];

    newpoint = translation  * oldpoint;
    newpoint = rotation * newpoint;

    newpoints(i,:) = newpoint;
    i = i + 1;
end

newpoints(:,3) = -1*newpoints(:,3)
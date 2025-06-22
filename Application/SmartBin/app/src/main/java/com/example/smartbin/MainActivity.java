package com.example.smartbin;

import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.util.Log;


import androidx.activity.EdgeToEdge;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;

public class MainActivity extends AppCompatActivity {
    private TextView fullness, lid_status, touch, weight, force;
    private Button lidButton;
    private Integer lidStatus = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_main);

        FirebaseDatabase database = FirebaseDatabase.getInstance();
        DatabaseReference dbref;
        DatabaseReference dbref2;
        fullness = findViewById(R.id.Fullness);
        weight = findViewById(R.id.Weight);
        touch = findViewById(R.id.Touch);
        lid_status = findViewById(R.id.lid_status);
        lidButton = findViewById(R.id.lid_status_button);
        force = findViewById(R.id.Force);

        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        dbref = database.getReference("Readings");
        dbref.child("fullness").addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot snapshot) {
                Object value = snapshot.getValue();
                if (value != null){
                    fullness.setText(value.toString());
                }
            }
            @Override
            public void onCancelled(@NonNull DatabaseError error) {
                Log.w("Failed to read value.", error.toException());
            }
        });
        dbref.child("weight").addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot snapshot) {
                Object value = snapshot.getValue();
                if (value != null){
                    weight.setText(value.toString());
                }
            }

            @Override
            public void onCancelled(@NonNull DatabaseError error) {
                Log.w("Failed to read value.", error.toException());
            }
        });
        dbref.child("touch").addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot snapshot) {
                Integer intValue = snapshot.getValue(Integer.class);

                if (intValue != null && intValue == 1) {
                    touch.setText("True");
                }else{
                    touch.setText("False");
                }
            }

            @Override
            public void onCancelled(@NonNull DatabaseError error) {
                Log.w("Failed to read value.", error.toException());
            }
        });
        dbref.child("lid_open").addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot snapshot) {
                Integer intValue = snapshot.getValue(Integer.class);

                if (intValue != null && intValue == 1) {
                    lid_status.setText("Lid is open");
                }else{
                    lid_status.setText("Lid is closed");
                }
            }
            @Override
            public void onCancelled(@NonNull DatabaseError error) {
                Log.w("Failed to read value.", error.toException());
            }
        });
        dbref2 = database.getReference("open_lid");
        lidButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (lidStatus == 1) {
                    dbref2.setValue(0);
                    lidStatus = 0;
                    force.setText("False");
                } else {
                    dbref2.setValue(1);
                    lidStatus = 1;
                    force.setText("True");
                }
            }
        });
    }
}
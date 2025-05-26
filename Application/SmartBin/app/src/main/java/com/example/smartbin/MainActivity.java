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
    private TextView fullness, lid_status;
    private Button lidButton;
    private boolean lidStatus;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_main);

        FirebaseDatabase database = FirebaseDatabase.getInstance();
        DatabaseReference dbref;
        fullness = findViewById(R.id.Fullness);
        lid_status = findViewById(R.id.lid_status);
        lidButton = findViewById(R.id.lid_status_button);

        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        dbref = database.getReference("Readings");
        dbref.child("Fullness").addValueEventListener(new ValueEventListener() {
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
        dbref = database.getReference("open_lid");
        DatabaseReference finalDbref = dbref;
        finalDbref.addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot snapshot) {
                lidStatus = Boolean.TRUE.equals(snapshot.getValue(Boolean.class));
                lid_status.setText(lidStatus ? "Open" : "Closed");
            }
            @Override
            public void onCancelled(@NonNull DatabaseError error) {
                Log.w("Failed to read value.", error.toException());
            }
        });
        lidButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (lidStatus) {
                    finalDbref.setValue(false);
                } else {
                    finalDbref.setValue(true);
                }
            }
        });
    }
}